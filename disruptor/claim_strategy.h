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
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FRANCOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef DISRUPTOR_CLAIM_STRATEGY_H_  // NOLINT
#define DISRUPTOR_CLAIM_STRATEGY_H_  // NOLINT

#include <thread>

#include "disruptor/sequence.h"

namespace disruptor {

enum ClaimStrategyOption { kSingleThreadedStrategy, kMultiThreadedStrategy };

// Optimised strategy can be used when there is a single publisher thread
// claiming {@link AbstractEvent}s.
template <size_t N>
class SingleThreadedStrategy {
 public:
  SingleThreadedStrategy() {}

  int64_t IncrementAndGet(const std::vector<Sequence*>& dependents) {
    const int64_t next_sequence = sequence_.IncrementAndGet(1L);
    WaitForFreeSlotAt(next_sequence, dependents);
    return next_sequence;
  }

  int64_t IncrementAndGet(const int& delta,
                          const std::vector<Sequence*>& dependents) {
    const int64_t next_sequence = sequence_.IncrementAndGet(delta);
    WaitForFreeSlotAt(next_sequence, dependents);
    return next_sequence;
  }

  bool HasAvalaibleCapacity(const std::vector<Sequence*>& dependents) {
    const int64_t wrap_point = sequence_.sequence() + 1L - N;
    if (wrap_point > min_gating_sequence_.sequence()) {
      const int64_t min_sequence = GetMinimumSequence(dependents);
      min_gating_sequence_.set_sequence(min_sequence);
      if (wrap_point > min_sequence) return false;
    }
    return true;
  }

  void SetSequence(const int64_t& sequence,
                   const std::vector<Sequence*>& dependents) {
    sequence_.set_sequence(sequence);
    WaitForFreeSlotAt(sequence, dependents);
  }

  void SerialisePublishing(const int64_t& sequence, const Sequence& cursor,
                           const int64_t& batch_size) {}

 private:
  void WaitForFreeSlotAt(const int64_t& sequence,
                         const std::vector<Sequence*>& dependents) {
    const int64_t wrap_point = sequence - N;
    if (min_gating_sequence_.sequence() < wrap_point) {
      while (GetMinimumSequence(dependents) < wrap_point) {
        std::this_thread::yield();
      }
    }
  }

  PaddedLong sequence_;
  PaddedLong min_gating_sequence_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(SingleThreadedStrategy);
};

// Strategy to be used when there are multiple publisher threads claiming
// {@link AbstractEvent}s.
template <size_t N>
class MultiThreadedStrategy {
 public:
  MultiThreadedStrategy() : sequence_(kInitialCursorValue) {}

  int64_t IncrementAndGet(const std::vector<Sequence*>& dependents) {
    WaitForCapacity(dependents, min_gating_sequence_local_);
    int64_t next_sequence = sequence_.IncrementAndGet();
    WaitForFreeSlotAt(next_sequence, dependents, min_gating_sequence_local_);
    return next_sequence;
  }

  int64_t IncrementAndGet(const int& delta,
                          const std::vector<Sequence*>& dependents) {
    int64_t next_sequence = sequence_.IncrementAndGet(delta);
    WaitForFreeSlotAt(next_sequence, dependents, min_gating_sequence_local_);
    return next_sequence;
  }
  void SetSequence(const int64_t& sequence,
                   const std::vector<Sequence*>& dependents) {
    sequence_.set_sequence(sequence);
    WaitForFreeSlotAt(sequence, dependents, min_gating_sequence_local_);
  }

  bool HasAvalaibleCapacity(const std::vector<Sequence*>& dependents) {
    const int64_t wrap_point = sequence_.sequence() + 1L - N;
    if (wrap_point > min_gating_sequence_local_.sequence()) {
      int64_t min_sequence = GetMinimumSequence(dependents);
      min_gating_sequence_local_.set_sequence(min_sequence);
      if (wrap_point > min_sequence) return false;
    }
    return true;
  }

  void SerialisePublishing(const Sequence& cursor, const int64_t& sequence,
                           const int64_t& batch_size) {
    int64_t expected_sequence = sequence - batch_size;
    int counter = retries;

    while (expected_sequence != cursor.sequence()) {
      if (0 == --counter) {
        counter = retries;
        std::this_thread::yield();
      }
    }
  }

 private:
  // Methods
  void WaitForCapacity(const std::vector<Sequence*>& dependents,
                       Sequence& min_gating_sequence) {
    const int64_t wrap_point = sequence_.sequence() + 1L - N;
    if (wrap_point > min_gating_sequence.sequence()) {
      int counter = retries;
      int64_t min_sequence;
      while (wrap_point > (min_sequence = GetMinimumSequence(dependents))) {
        counter = ApplyBackPressure(counter);
      }
      min_gating_sequence.set_sequence(min_sequence);
    }
  }

  void WaitForFreeSlotAt(const int64_t& sequence,
                         const std::vector<Sequence*>& dependents,
                         Sequence& min_gating_sequence) {
    const int64_t wrap_point = sequence - N;
    if (wrap_point > min_gating_sequence.sequence()) {
      int64_t min_sequence;
      while (wrap_point > (min_sequence = GetMinimumSequence(dependents))) {
        std::this_thread::yield();
      }
      min_gating_sequence.set_sequence(min_sequence);
    }
  }

  int ApplyBackPressure(int counter) {
    if (0 != counter) {
      --counter;
      std::this_thread::yield();
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return counter;
  }

  PaddedSequence min_gating_sequence_local_;
  PaddedSequence sequence_;
  // TODO: make thread_local
  PaddedSequence min_gating_sequence_;

  const int retries = 100;

  DISALLOW_COPY_MOVE_AND_ASSIGN(MultiThreadedStrategy);
};

};  // namespace disruptor

#endif  // DISRUPTOR_CLAIM_STRATEGY_H_ NOLINT
