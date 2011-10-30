// Copyright (c) 2011, François Saint-Jacques
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the disruptor-- nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FRANÇOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SequencerTest

#include <atomic>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include <disruptor/sequencer.h>

#define BUFFER_SIZE 4

namespace disruptor {
namespace test {

struct SequencerFixture {
    SequencerFixture() :
        sequencer(BUFFER_SIZE,
                  kSingleThreadedStrategy,
                  kSleepingStrategy),
        gating_sequence(kInitialCursorValue) {
            std::vector<Sequence*> sequences;
            sequences.push_back(&gating_sequence);
            sequencer.set_gating_sequences(sequences);
        }

    ~SequencerFixture() {}

    void FillBuffer() {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int64_t sequence = sequencer.Next();
            sequencer.Publish(sequence);
        }
    }

    Sequencer sequencer;
    Sequence gating_sequence;
};

BOOST_AUTO_TEST_SUITE(SequencerBasic)

BOOST_FIXTURE_TEST_CASE(ShouldStartWithValueInitialized, SequencerFixture) {
    BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue);
}

BOOST_FIXTURE_TEST_CASE(ShouldGetPublishFirstSequence, SequencerFixture) {
    const int64_t sequence = sequencer.Next();
    BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue);
    BOOST_CHECK(sequence == 0);

    sequencer.Publish(sequence);
    BOOST_CHECK(sequencer.GetCursor() == sequence);
}

BOOST_FIXTURE_TEST_CASE(ShouldIndicateAvailableCapacity, SequencerFixture) {
    BOOST_CHECK(sequencer.HasAvalaibleCapacity());
}

BOOST_FIXTURE_TEST_CASE(ShouldIndicateNoAvailableCapacity, SequencerFixture) {
    FillBuffer();
    BOOST_CHECK(sequencer.HasAvalaibleCapacity() == false);
}

BOOST_FIXTURE_TEST_CASE(ShouldForceClaimSequence, SequencerFixture) {
    const int64_t claim_sequence = 3;
    const int64_t sequence = sequencer.Claim(claim_sequence);

    BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue);
    BOOST_CHECK(sequence == claim_sequence);

    sequencer.ForcePublish(sequence);
    BOOST_CHECK(sequencer.GetCursor() == claim_sequence);
}

BOOST_FIXTURE_TEST_CASE(ShouldPublishSequenceBatch, SequencerFixture) {
    const int batch_size = 3;
    BatchDescriptor batch_descriptor(batch_size);
    sequencer.Next(&batch_descriptor);

    BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue);
    BOOST_CHECK(batch_descriptor.end() == kInitialCursorValue + batch_size);
    BOOST_CHECK(batch_descriptor.size() == batch_size);

    sequencer.Publish(batch_descriptor);
    BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue + batch_size);
}

BOOST_FIXTURE_TEST_CASE(ShouldWaitOnSequence, SequencerFixture) {
    std::vector<Sequence*> dependents(0);
    ProcessingSequenceBarrier* barrier = sequencer.NewBarrier(dependents);

    const int64_t sequence = sequencer.Next();
    sequencer.Publish(sequence);

    BOOST_CHECK(sequence == barrier->WaitFor(sequence));
}

BOOST_FIXTURE_TEST_CASE(ShouldWaitOnSequenceShowingBatchingEffect, SequencerFixture) {
    std::vector<Sequence*> dependents(0);
    ProcessingSequenceBarrier* barrier = sequencer.NewBarrier(dependents);

    sequencer.Publish(sequencer.Next());
    sequencer.Publish(sequencer.Next());

    const int64_t sequence = sequencer.Next();
    sequencer.Publish(sequence);

    BOOST_CHECK(sequence == barrier->WaitFor(kInitialCursorValue + 1LL));
}

BOOST_FIXTURE_TEST_CASE(ShouldSignalWaitingProcessorWhenSequenceIsPublished, SequencerFixture) {
    std::vector<Sequence*> dependents(0);
    ProcessingSequenceBarrier* barrier = sequencer.NewBarrier(dependents);

    std::atomic<bool> waiting(true);
    std::atomic<bool> completed(false);


    std::thread thread(
            // lambda prototype
            [](Sequence* gating_sequence,
               ProcessingSequenceBarrier* barrier,
               std::atomic<bool>* waiting,
               std::atomic<bool>* completed) {
            // body
                waiting->store(false);
                BOOST_CHECK(kInitialCursorValue + 1LL == \
                    barrier->WaitFor(kInitialCursorValue + 1LL));
                gating_sequence->set_sequence(kInitialCursorValue + 1LL);
                completed->store(true);
            },
            &gating_sequence,
            barrier,
            &waiting,
            &completed);

    while (waiting.load()) {}
    BOOST_CHECK(gating_sequence.sequence() == kInitialCursorValue);

    sequencer.Publish(sequencer.Next());

    while (!completed.load()) {}
    BOOST_CHECK(gating_sequence.sequence() == kInitialCursorValue + 1LL);

    thread.join();
}

BOOST_FIXTURE_TEST_CASE(ShouldHoldUpPublisherWhenRingIsFull, SequencerFixture) {
    std::atomic<bool> waiting(true);
    std::atomic<bool> completed(false);

    FillBuffer();

    const int64_t expected_full_cursor = kInitialCursorValue + BUFFER_SIZE;
    BOOST_CHECK(sequencer.GetCursor() == expected_full_cursor);

    std::thread thread(
            // lambda prototype
            [](Sequencer* sequencer,
               std::atomic<bool>* waiting,
               std::atomic<bool>* completed) {
                // body
                waiting->store(false);
                sequencer->Publish(sequencer->Next());
                completed->store(true);
            }, // end of lambda
            &sequencer,
            &waiting,
            &completed);

    while (waiting.load()) {}
    BOOST_CHECK(sequencer.GetCursor() == expected_full_cursor);

    gating_sequence.set_sequence(kInitialCursorValue + 1LL);

    while (!completed.load()) {}
    BOOST_CHECK(sequencer.GetCursor() == expected_full_cursor + 1LL);

    thread.join();

}

BOOST_AUTO_TEST_SUITE_END()

}; // namepspace test
}; // namepspace disruptor
