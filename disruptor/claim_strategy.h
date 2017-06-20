// Copyright (c) 2011-2015, Francois Saint-Jacques
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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef DISRUPTOR_CLAIM_STRATEGY_H_  // NOLINT
#define DISRUPTOR_CLAIM_STRATEGY_H_  // NOLINT

#include <thread>

#include "disruptor/ring_buffer.h"
#include "disruptor/sequence.h"

namespace disruptor {

/*
// Strategy employed by a {@link Publisher} to wait claim and publish sequences
// on the sequencer.
//
class ClaimStrategy {
 public:
  // Wait for the given sequence to be available for consumption.
  //
  // @param dependents  dependents sequences to wait on (mostly consumers).
  // @param delta       sequences to claim [default: 1].
  //
  // @return last claimed sequence.
  int64_t IncrementAndGet(const std::vector<Sequence*>& dependents,
                          size_t delta = 1);

  // Verify in a non-blocking way that there exists claimable sequences.
  //
  // @param dependents  dependents sequences to wait on (mostly consumers).
  //
  // @return last claimed sequence.
  bool HasAvailableCapacity(const std::vector<Sequence*>& dependents);

  void SynchronizePublishing(const int64_t& sequence, const Sequence& cursor,
                             const size_t& delta) {}
};
*/

class SingleThreadedStrategy;
using kDefaultClaimStrategy = SingleThreadedStrategy;

// Optimised strategy can be used when there is a single publisher thread.
class SingleThreadedStrategy {
 public:
  SingleThreadedStrategy(size_t n)
      : buffer_size_(n),
        last_claimed_sequence_(kInitialCursorValue),
        last_consumer_sequence_(kInitialCursorValue) {}

  int64_t IncrementAndGet(Sequence& cursor,
                          const std::vector<Sequence*>& dependents,
                          size_t delta = 1) {
    const int64_t next_sequence = (last_claimed_sequence_ += delta);
    const int64_t wrap_point = next_sequence - buffer_size_;
    if (last_consumer_sequence_ < wrap_point) {
      while (GetMinimumSequence(dependents) < wrap_point) {
        // TODO: configurable yield strategy
        std::this_thread::yield();
      }
    }
    return next_sequence;
  }

  bool HasAvailableCapacity(const std::vector<Sequence*>& dependents,
                            int64_t cursor, int required_capacity = 1) {
    const int64_t wrap_point = last_claimed_sequence_ + required_capacity - buffer_size_;
    if (wrap_point > last_consumer_sequence_) {
      const int64_t min_sequence = GetMinimumSequence(dependents);
      last_consumer_sequence_ = min_sequence;
      if (wrap_point > min_sequence) return false;
    }
    return true;
  }

  void SynchronizePublishing(const int64_t& sequence, Sequence& cursor,
                             const size_t& delta) {
    cursor.set_sequence(sequence);
  }

  int64_t GetHighestPublishedSequence(int64_t lowerBound,
                                      int64_t availableSequence) {
    return availableSequence;
  }

 private:
  // We do not need to use atomic values since this function is called by a
  // single publisher.
  size_t buffer_size_;
  int64_t last_claimed_sequence_;
  int64_t last_consumer_sequence_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(SingleThreadedStrategy);
};

// Optimised strategy can be used when there is a single publisher thread.
class MultiThreadedStrategy {
 public:
  MultiThreadedStrategy(size_t n) : buffer_size_(n) {}

  int64_t IncrementAndGet(Sequence& cursor,
                          const std::vector<Sequence*>& dependents,
                          size_t delta = 1) {
    const int64_t next_sequence = last_claimed_sequence_.IncrementAndGet(delta);
    const int64_t wrap_point = next_sequence - buffer_size_;
    if (last_consumer_sequence_.sequence() < wrap_point) {
      while (GetMinimumSequence(dependents) < wrap_point) {
        // TODO: configurable yield strategy
        std::this_thread::yield();
      }
    }
    return next_sequence;
  }

  bool HasAvailableCapacity(const std::vector<Sequence*>& dependents,
                            int64_t cursor, int required_capacity = 1) {
    const int64_t wrap_point =
        last_claimed_sequence_.sequence() + required_capacity - buffer_size_;
    if (wrap_point > last_consumer_sequence_.sequence()) {
      const int64_t min_sequence = GetMinimumSequence(dependents);
      last_consumer_sequence_.set_sequence(min_sequence);
      if (wrap_point > min_sequence) return false;
    }
    return true;
  }

  void SynchronizePublishing(const int64_t& sequence, Sequence& cursor,
                             const size_t& delta) {
    int64_t my_first_sequence = sequence - delta;

    while (cursor.sequence() < my_first_sequence) {
      // TODO: configurable yield strategy
      std::this_thread::yield();
    }

    cursor.set_sequence(sequence);
  }

  int64_t GetHighestPublishedSequence(int64_t lowerBound,
                                      int64_t availableSequence) {
    return availableSequence;
  }

 private:
  size_t buffer_size_;
  Sequence last_claimed_sequence_;
  Sequence last_consumer_sequence_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(MultiThreadedStrategy);
};

// Modelled after MultiProducerSequencer in LMAX disruptor
class MultiThreadedStrategyEx {
 public:
  MultiThreadedStrategyEx(size_t n)
      : buffer_size_(n), index_mask_(n - 1), index_shift_(log2(n)), available_buffer_(n) {
    for (size_t i = 0; i < n; ++i)
      available_buffer_[i].store(-1, std::memory_order_release);
  }

  int64_t IncrementAndGet(Sequence& cursor,
                          const std::vector<Sequence*>& dependents,
                          size_t delta = 1) {
    int64_t current;
    int64_t next;

    do {
      current = cursor.sequence();
      next = current + delta;

      int64_t wrap_point = next - buffer_size_;
      int64_t cached_gating_sequence = last_consumer_sequence_.sequence();

      if (wrap_point > cached_gating_sequence ||
          cached_gating_sequence > current) {
        int64_t gating_sequence = GetMinimumSequence(dependents, current);

        if (wrap_point > gating_sequence) {
          // TODO: configurable yield strategy
          std::this_thread::yield();
          continue;
        }

        last_consumer_sequence_.set_sequence(gating_sequence);
      } else if (cursor.compare_and_swap(current, next)) {
        break;
      }
    } while (true);

    return next;
  }

  bool HasAvailableCapacity(const std::vector<Sequence*>& dependents,
                            int64_t cursor_value, int required_capacity = 1) {
    int64_t wrap_point = (cursor_value + required_capacity) - buffer_size_;
    int64_t cached_gating_sequence = last_consumer_sequence_.sequence();

    if (wrap_point > cached_gating_sequence ||
        cached_gating_sequence > cursor_value) {
      int64_t min_sequence = GetMinimumSequence(dependents, cursor_value);
      last_consumer_sequence_.set_sequence(min_sequence);

      if (wrap_point > min_sequence) {
        return false;
      }
    }

    return true;
  }

  void SynchronizePublishing(const int64_t& sequence, Sequence& cursor,
                             const size_t& delta) {
    int64_t my_first_sequence = sequence - delta + 1;

    for (int64_t l = my_first_sequence; l <= sequence; l++) {
      setAvailable(l);
    }
  }

  int64_t GetHighestPublishedSequence(int64_t lowerBound,
                                      int64_t availableSequence) {
    for (int64_t sequence = lowerBound; sequence <= availableSequence;
         sequence++) {
      if (!isAvailable(sequence)) {
        return sequence - 1;
      }
    }

    return availableSequence;
  }

 private:
  int calculateAvailabilityFlag(int64_t sequence) {
    return (int)(sequence >> index_shift_);
  }

  void setAvailable(int64_t sequence) {
    setAvailableBufferValue((sequence & index_mask_),
                            calculateAvailabilityFlag(sequence));
  }

  void setAvailableBufferValue(size_t index, int flag) {
    available_buffer_[index].store(flag, std::memory_order_release);
  }

  bool isAvailable(int64_t sequence) {
    size_t index = (sequence & index_mask_);
    int flag = calculateAvailabilityFlag(sequence);

    return (available_buffer_[index].load(std::memory_order_acquire) == flag);
  }

  size_t buffer_size_;
  int index_mask_;
  int index_shift_;
  Sequence last_consumer_sequence_;
  std::vector<std::atomic<int> > available_buffer_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(MultiThreadedStrategyEx);
};

};  // namespace disruptor

#endif  // DISRUPTOR_CLAIM_STRATEGY_H_ NOLINT
