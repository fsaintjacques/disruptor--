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

template<typename T>
class RingBuffer : public Sequencer {
 public:
    RingBuffer(EventFactoryInterface<T>* event_factory,
               int buffer_size,
               ClaimStrategyOption claim_strategy_option,
               WaitStrategyOption wait_strategy_option) : 
            Sequencer(buffer_size,
                      claim_strategy_option,
                      wait_strategy_option),
            buffer_size_(buffer_size),
            mask_(buffer_size - 1),
            events_(buffer_size) {
        Fill(event_factory);
    }

    T* Get(const int64_t& sequence) {
        return &events_[sequence & mask_];
    }

 private:
    void Fill(EventFactoryInterface<T>* event_factory) {
        for (int i = 0 ; i < buffer_size_; i++) {
            events_[i] = event_factory->NewInstance();
        }
    }

    // Members
    int buffer_size_;
    int mask_;
    std::vector<T> events_;
};

};  // namespace disruptor

#endif // DISRUPTOR_RING_BUFFER_H_ NOLINT
