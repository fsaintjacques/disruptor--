// Copyright 2011 <François Saint-Jacques>

#include <array>
#include <vector>

#include "disruptor/interface.h"
#include "disruptor/claim_strategy.h"
#include "disruptor/wait_strategy.h"
#include "disruptor/sequencer.h"
#include "disruptor/sequence_barrier.h"

#ifndef DISRUPTOR_RING_BUFFER_H_ // NOLINT
#define DISRUPTOR_RING_BUFFER_H_ // NOLINT

namespace disruptor {

// Ring based store of reusable entries containing the data representing an
// event beign exchanged between publisher and {@link EventProcessor}s.
//
// @param <T> implementation storing the data for sharing during exchange
// or parallel coordination of an event.
template<typename T>
class RingBuffer : public Sequencer {
 public:
    // Construct a RingBuffer with the full option set.
    //
    // @param event_factory to instance new entries for filling the RingBuffer.
    // @param size of the RingBuffer that will be rounded up to the next power
    // of 2.
    // @param claim_strategy_option threading strategy for publishers claiming
    // entries in the ring.
    // @param wait_strategy_option waiting strategy employed by
    // processors_to_track waiting in entries becoming available.
    RingBuffer(EventFactoryInterface<T>* event_factory,
               int buffer_size,
               ClaimStrategyOption claim_strategy_option,
               WaitStrategyOption wait_strategy_option) :
            Sequencer(buffer_size,
                      claim_strategy_option,
                      wait_strategy_option),
            buffer_size_(buffer_size),
            mask_(buffer_size - 1),
            events_(event_factory->NewInstance(buffer_size)) {
    }

    ~RingBuffer() {
        delete[] events_;
    }

    // Get the event for a given sequence in the RingBuffer.
    //
    // @param sequence for the event
    // @return event pointer at the specified sequence position.
    T* Get(const int64_t& sequence) {
        return &events_[sequence & mask_];
    }

 private:
    // Members
    int buffer_size_;
    int mask_;
    T* events_;
};

};  // namespace disruptor

#endif // DISRUPTOR_RING_BUFFER_H_ NOLINT
