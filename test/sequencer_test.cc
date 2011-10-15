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
            uint64_t sequence = sequencer.Next();
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
    const uint64_t sequence = sequencer.Next();
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

BOOST_AUTO_TEST_SUITE_END()
