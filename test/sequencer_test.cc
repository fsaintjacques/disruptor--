// Copyright 2011 <François Saint-Jacques>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SequencerTest

#include <boost/test/unit_test.hpp>

#include <disruptor/sequencer.h>

#define BUFFER_SIZE 4

struct SequencerFixture {
    SequencerFixture() :
        sequencer(BUFFER_SIZE,
                  disruptor::kSingleThreadedStrategy,
                  disruptor::kSleepingStrategy),
        gating_sequence(disruptor::kInitialCursorValue) {
            std::vector<disruptor::Sequence*> sequences;
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

    disruptor::Sequencer sequencer;
    disruptor::Sequence gating_sequence;
};

BOOST_AUTO_TEST_SUITE(SequencerBasic)

BOOST_FIXTURE_TEST_CASE(ShouldStartWithValueInitialized, SequencerFixture) {
    BOOST_CHECK(sequencer.GetCursor() == disruptor::kInitialCursorValue);
}

BOOST_FIXTURE_TEST_CASE(ShouldGetPublishFirstSequence, SequencerFixture) {
    const int64_t sequence = sequencer.Next();
    BOOST_CHECK(sequencer.GetCursor() == disruptor::kInitialCursorValue);
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

    BOOST_CHECK(sequencer.GetCursor() == disruptor::kInitialCursorValue);
    BOOST_CHECK(sequence == claim_sequence);

    sequencer.ForcePublish(sequence);
    BOOST_CHECK(sequencer.GetCursor() == claim_sequence);
}

BOOST_FIXTURE_TEST_CASE(ShouldPublishSequenceBatch, SequencerFixture) {
    const int batch_size = 3;
    disruptor::BatchDescriptor batch_descriptor(batch_size);
    sequencer.Next(&batch_descriptor);

    BOOST_CHECK(sequencer.GetCursor() == disruptor::kInitialCursorValue);
    BOOST_CHECK(batch_descriptor.end() == disruptor::kInitialCursorValue + batch_size);
    BOOST_CHECK(batch_descriptor.size() == batch_size);

    sequencer.Publish(batch_descriptor);
    BOOST_CHECK(sequencer.GetCursor() == disruptor::kInitialCursorValue + batch_size);
}

BOOST_FIXTURE_TEST_CASE(ShouldWaitOnSequence, SequencerFixture) {
    std::vector<disruptor::Sequence*> dependents(0);
    disruptor::ProcessingSequenceBarrier* barrier = sequencer.NewBarrier(dependents);

    const int64_t sequence = sequencer.Next();
    sequencer.Publish(sequence);

    BOOST_CHECK(sequence == barrier->WaitFor(sequence));
}

BOOST_FIXTURE_TEST_CASE(ShouldWaitOnSequenceShowingBatchingEffect, SequencerFixture) {
    std::vector<disruptor::Sequence*> dependents(0);
    disruptor::ProcessingSequenceBarrier* barrier = sequencer.NewBarrier(dependents);

    sequencer.Publish(sequencer.Next());
    sequencer.Publish(sequencer.Next());

    const int64_t sequence = sequencer.Next();
    sequencer.Publish(sequence);

    BOOST_CHECK(sequence == barrier->WaitFor(disruptor::kInitialCursorValue + 1LL));
}

BOOST_FIXTURE_TEST_CASE(ShouldSignalWaitingProcessorWhenSequenceIsPublished, SequencerFixture) {
    // TODO(fsaintjacques): implement this method with mutexes
}

BOOST_FIXTURE_TEST_CASE(ShouldHoldUpPublisherWhenRingIsFull, SequencerFixture) {
    // TODO(fsaintjacques): implement this method with mutexes
}

BOOST_AUTO_TEST_SUITE_END()
