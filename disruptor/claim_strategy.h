// Copyright 2011 <François Saint-Jacques>

#include <thread>
#include <vector>

#include "disruptor/interface.h"

#ifndef DISRUPTOR_CLAIM_STRATEGY_H_ // NOLINT
#define DISRUPTOR_CLAIM_STRATEGY_H_ // NOLINT

namespace disruptor {

enum ClaimStrategyOption {
    kSingleThreadedStrategy,
    kMultiThreadedStrategy
};

// Optimised strategy can be used when there is a single publisher thread
// claiming {@link AbstractEvent}s.
class SingleThreadedStrategy :  public ClaimStrategyInterface {
 public:
    SingleThreadedStrategy(const int& buffer_size) :
        buffer_size_(buffer_size),
        sequence_(kInitialCursorValue),
        min_processor_sequence_(kInitialCursorValue) {}

    virtual int64_t IncrementAndGet(const int64_t& delta) {
        sequence_ += delta;
        return sequence_;
    }

    virtual int64_t IncrementAndGet() {
        return ++sequence_;
    }

    virtual void SetSequence(const int64_t& sequence) {
        sequence_ = sequence;
    }

    virtual void EnsureProcessorsAreInRange(const int64_t& sequence,
        const std::vector<Sequence*>& dependent_sequences) {
        int64_t wrap_point = sequence - buffer_size_;
        if (wrap_point > min_processor_sequence_) {
            int64_t min_sequence = GetMinimumSequence(dependent_sequences);
            while (wrap_point > min_sequence) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                min_sequence = GetMinimumSequence(dependent_sequences);
            }
            min_processor_sequence_ = min_sequence;
        }
    }

    virtual void SerialisePublishing(const Sequence& cursor,
                                     const int64_t& sequence,
                                     const int64_t& batch_size) {}

 private:
    SingleThreadedStrategy();

    const int buffer_size_;
    int64_t sequence_;
    int64_t min_processor_sequence_;

    DISALLOW_COPY_AND_ASSIGN(SingleThreadedStrategy);
};

// Strategy to be used when there are multiple publisher threads claiming
// {@link AbstractEvent}s.
class MultiThreadedStrategy :  public ClaimStrategyInterface {
 public:
    MultiThreadedStrategy(const int& buffer_size) :
        buffer_size_(buffer_size),
        sequence_(kInitialCursorValue),
        min_processor_sequence_(kInitialCursorValue) {}

    virtual int64_t IncrementAndGet(const int64_t& delta) {
        return sequence_.IncrementAndGet(delta);
    }

    virtual int64_t IncrementAndGet() {
        return IncrementAndGet(1L);
    }

    virtual void SetSequence(const int64_t& sequence) {
        return sequence_.set_sequence(sequence);
    }

    virtual void EnsureProcessorsAreInRange(const int64_t& sequence,
        const std::vector<Sequence*>& dependent_sequences) {
        int64_t wrap_point = sequence - buffer_size_;
        if (wrap_point > min_processor_sequence_.sequence()) {
            int64_t min_sequence = GetMinimumSequence(dependent_sequences);
            while (wrap_point > min_sequence) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                min_sequence = GetMinimumSequence(dependent_sequences);
            }
            min_processor_sequence_.set_sequence(min_sequence);
        }
    }

    virtual void SerialisePublishing(const Sequence& cursor,
                                     const int64_t& sequence,
                                     const int64_t& batch_size) {
        int64_t expected_sequence = sequence - batch_size;
        int counter = 1000;

        while (expected_sequence != cursor.sequence()) {
            if (0 == --counter) {
                counter = 1000;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

 private:
    MultiThreadedStrategy();

    const int buffer_size_;
    Sequence sequence_;
    Sequence min_processor_sequence_;

    DISALLOW_COPY_AND_ASSIGN(MultiThreadedStrategy);
};

ClaimStrategyInterface* CreateClaimStrategy(ClaimStrategyOption option,
                                            const int& buffer_size) {
    switch (option) {
        case kSingleThreadedStrategy:
            return new SingleThreadedStrategy(buffer_size);
        case kMultiThreadedStrategy:
            return new MultiThreadedStrategy(buffer_size);
        default:
            return NULL;
    }
};

};  // namespace disruptor

#endif // DISRUPTOR_CLAIM_STRATEGY_H_ NOLINT
