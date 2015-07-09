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

#ifndef CACHE_LINE_SIZE_IN_BYTES     // NOLINT
#define CACHE_LINE_SIZE_IN_BYTES 64  // NOLINT
#endif                               // NOLINT
#define ATOMIC_SEQUENCE_PADDING_LENGTH \
  (CACHE_LINE_SIZE_IN_BYTES - sizeof(std::atomic<int64_t>)) / 8
#define SEQUENCE_PADDING_LENGTH (CACHE_LINE_SIZE_IN_BYTES - sizeof(int64_t)) / 8

#ifndef DISRUPTOR_SEQUENCE_H_  // NOLINT
#define DISRUPTOR_SEQUENCE_H_  // NOLINT

#include <atomic>

#include "disruptor/utils.h"

namespace disruptor {

// special cursor values
constexpr int64_t kInitialCursorValue = -1L;
constexpr int64_t kAlertedSignal = -2L;
constexpr int64_t kTimeoutSignal = -3L;
constexpr int64_t kFirstSequenceValue = kInitialCursorValue + 1L;

// Sequence counter.
class Sequence {
 public:
  // Construct a sequence counter that can be tracked across threads.
  //
  // @param initial_value for the counter.
  Sequence(int64_t initial_value = kInitialCursorValue)
      : sequence_(initial_value) {}

  // Get the current value of the {@link Sequence}.
  //
  // @return the current value.
  int64_t sequence() const {
    return sequence_.load(std::memory_order::memory_order_acquire);
  }

  // Set the current value of the {@link Sequence}.
  //
  // @param the value to which the {@link Sequence} will be set.
  void set_sequence(int64_t value) {
    sequence_.store(value, std::memory_order::memory_order_release);
  }

  // Increment and return the value of the {@link Sequence}.
  //
  // @param increment the {@link Sequence}.
  // @return the new value incremented.
  int64_t IncrementAndGet(const int64_t& increment) {
    return sequence_.fetch_add(increment,
                               std::memory_order::memory_order_release) +
           increment;
  }

 private:
  // padding
  int64_t padding0_[ATOMIC_SEQUENCE_PADDING_LENGTH];
  // members
  std::atomic<int64_t> sequence_;
  // padding
  int64_t padding1_[ATOMIC_SEQUENCE_PADDING_LENGTH];

  DISALLOW_COPY_MOVE_AND_ASSIGN(Sequence);
};

int64_t GetMinimumSequence(const std::vector<Sequence*>& sequences) {
  int64_t minimum = LONG_MAX;

  for (Sequence* sequence_ : sequences) {
    const int64_t sequence = sequence_->sequence();
    minimum = minimum < sequence ? minimum : sequence;
  }

  return minimum;
};

};  // namespace disruptor

#endif  // DISRUPTOR_SEQUENCE_H_ NOLINT
