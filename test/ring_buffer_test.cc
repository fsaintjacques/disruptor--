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
#define BOOST_TEST_MODULE RingBufferTest

#include <exception>
#include <future>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <disruptor/event_processor.h>
#include <disruptor/ring_buffer.h>

#include "support/stub_event.h"

#define BUFFER_SIZE 64

namespace disruptor {
namespace test {

struct RingBufferFixture {
    RingBufferFixture() :
        factory(new StubEventFactory()),
        ring_buffer(factory.get(),
                    BUFFER_SIZE,
                    disruptor::kSingleThreadedStrategy,
                    disruptor::kSleepingStrategy),
        stub_processor(&ring_buffer) {
            std::vector<disruptor::Sequence*> sequences;
            sequences.push_back(stub_processor.GetSequence());
            ring_buffer.set_gating_sequences(sequences);
            std::vector<Sequence*> empty_sequences(0);
            barrier = (SequenceBarrierInterface*) ring_buffer.NewBarrier(empty_sequences);
        }

    ~RingBufferFixture() {}

    void FillBuffer() {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int64_t sequence = ring_buffer.Next();
            ring_buffer.Publish(sequence);
        }
    }


    std::unique_ptr<StubEventFactory> factory;
    RingBuffer<StubEvent> ring_buffer;
    NoOpEventProcessor<StubEvent> stub_processor;
    SequenceBarrierInterface* barrier;
};

std::vector<StubEvent> Waiter(RingBuffer<StubEvent>* ring_buffer,
                              SequenceBarrierInterface* barrier,
                              int64_t initial_sequence,
                              int64_t to_wait_for_sequence) {
    barrier->WaitFor(to_wait_for_sequence);

    std::vector<StubEvent> results;
    for (int64_t i = initial_sequence; i <= to_wait_for_sequence; i++)
        results.push_back(*ring_buffer->Get(i));

    return results;
};



class TestEventProcessor : public EventProcessorInterface<StubEvent> {
 public:
    TestEventProcessor(SequenceBarrierInterface* barrier) :
        barrier_(barrier) , sequence_(kInitialCursorValue) {}

    virtual Sequence* GetSequence() { return &sequence_; }

    virtual void Halt() {}
    virtual void Run() {
        try {
            barrier_->WaitFor(0L);
        } catch(...) {
            throw std::runtime_error("catched exception in TestEventProcessor::Run()");
        }

        sequence_.set_sequence(sequence_.sequence() + 1L);
    }

 private:
    PaddedSequence sequence_;
    SequenceBarrierInterface* barrier_;
};

BOOST_AUTO_TEST_SUITE(RingBufferBasic)

BOOST_FIXTURE_TEST_CASE(ShouldClaimAndGet, RingBufferFixture) {
    BOOST_CHECK(ring_buffer.GetCursor() == disruptor::kInitialCursorValue);
    StubEvent expected_event(1234);

    int64_t claim_sequence = ring_buffer.Next();
    StubEvent* old_event = ring_buffer.Get(claim_sequence);
    old_event->set_value(expected_event.value());
    ring_buffer.Publish(claim_sequence);

    int64_t sequence = barrier->WaitFor(0);
    BOOST_CHECK(sequence == 0);

    StubEvent* event = ring_buffer.Get(sequence);
    BOOST_CHECK(event->value() == expected_event.value());

    BOOST_CHECK(ring_buffer.GetCursor() == 0);
}

BOOST_FIXTURE_TEST_CASE(ShouldClaimAndGetWithTimeout, RingBufferFixture) {
    BOOST_CHECK(ring_buffer.GetCursor() == disruptor::kInitialCursorValue);
    StubEvent expected_event(1234);

    int64_t claim_sequence = ring_buffer.Next();
    StubEvent* old_event = ring_buffer.Get(claim_sequence);
    old_event->set_value(expected_event.value());
    ring_buffer.Publish(claim_sequence);

    int64_t sequence = barrier->WaitFor(0, 5000);
    BOOST_CHECK(sequence == 0);

    StubEvent* event = ring_buffer.Get(sequence);
    BOOST_CHECK(event->value() == expected_event.value());

    BOOST_CHECK(ring_buffer.GetCursor() == 0);
}

BOOST_FIXTURE_TEST_CASE(ShouldGetWithTimeout, RingBufferFixture) {
    int64_t sequence = barrier->WaitFor(0, 5000);
    BOOST_CHECK(sequence == kInitialCursorValue);
}

BOOST_FIXTURE_TEST_CASE(ShouldClaimAndGetInSeperateThread, RingBufferFixture) {
    std::future<std::vector<StubEvent>> future = \
        std::async(std::bind(&Waiter, &ring_buffer, barrier, 0LL, 0LL));

    StubEvent expected_event(1234);

    int64_t sequence = ring_buffer.Next();
    StubEvent* old_event = ring_buffer.Get(sequence);
    old_event->set_value(expected_event.value());
    ring_buffer.Publish(sequence);

    std::vector<StubEvent> results = future.get();

    BOOST_CHECK(results[0].value() == expected_event.value());
}

BOOST_FIXTURE_TEST_CASE(ShouldWrap, RingBufferFixture) {
    int n_messages = BUFFER_SIZE;
    int offset = 1000;

    for (int i = 0; i < n_messages + offset; i++) {
        int64_t sequence = ring_buffer.Next();
        StubEvent* event = ring_buffer.Get(sequence);
        event->set_value(i);
        ring_buffer.Publish(sequence);
    }

    int expected_sequence = n_messages + offset - 1;
    int64_t avalaible= barrier->WaitFor(expected_sequence);
    BOOST_CHECK(avalaible == expected_sequence);

    for (int i = offset; i < n_messages; i++) {
        BOOST_CHECK(i == ring_buffer.Get(i)->value());
    }
}

BOOST_FIXTURE_TEST_CASE(ShouldGetAtSpecificSequence, RingBufferFixture) {
    int64_t expected_sequence = 5;

    ring_buffer.Claim(expected_sequence);
    StubEvent* expected_event = ring_buffer.Get(expected_sequence);
    expected_event->set_value((int) expected_sequence);
    ring_buffer.ForcePublish(expected_sequence);

    int64_t sequence = barrier->WaitFor(expected_sequence);
    BOOST_CHECK(expected_sequence == sequence);

    StubEvent* event = ring_buffer.Get(sequence);
    BOOST_CHECK(expected_event->value() == event->value());

    BOOST_CHECK(expected_sequence == ring_buffer.GetCursor());
}

// Publisher will try to publish BUFFER_SIZE + 1 events. The last event
// should wait for at least one consume before publishing, thus preventing
// an overwrite. After the single consume, the publisher should resume and 
// publish the last event.
BOOST_FIXTURE_TEST_CASE(ShouldPreventPublishersOvertakingEventProcessorWrapPoint, RingBufferFixture) {
    std::atomic<bool> publisher_completed(false);
    std::atomic<int> counter(0);
    std::vector<Sequence*> dependency(0);
    TestEventProcessor processor((SequenceBarrierInterface*) ring_buffer.NewBarrier(dependency));
    dependency.push_back(processor.GetSequence());
    ring_buffer.set_gating_sequences(dependency);

    // Publisher in a seperate thread
    std::thread thread(
            // lambda definition
            [](RingBuffer<StubEvent>* ring_buffer,
               SequenceBarrierInterface* barrier,
               std::atomic<bool>* publisher_completed,
               std::atomic<int>* counter) {
            // body
                for (int i = 0; i <= BUFFER_SIZE; i++) {
                    int64_t sequence = ring_buffer->Next();
                    StubEvent* event = ring_buffer->Get(sequence);
                    event->set_value(i);
                    ring_buffer->Publish(sequence);
                    counter->fetch_add(1L);
                }

                publisher_completed->store(true);
            }, // end of lambda
            &ring_buffer,
            barrier,
            &publisher_completed,
            &counter);

    while (counter.load() < BUFFER_SIZE) {}

    int64_t sequence = ring_buffer.GetCursor();
    BOOST_CHECK(sequence == (BUFFER_SIZE - 1));
    BOOST_CHECK(publisher_completed.load() == false);

    processor.Run();
    thread.join();

    BOOST_CHECK(publisher_completed.load());
}

BOOST_AUTO_TEST_SUITE_END()

}; // namespace test
}; // namespace disruptor
