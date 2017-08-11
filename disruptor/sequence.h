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

#ifndef CACHE_LINE_SIZE_IN_BYTES // NOLINT
#define CACHE_LINE_SIZE_IN_BYTES 64 // NOLINT
#endif // NOLINT
#define ATOMIC_SEQUENCE_PADDING_LENGTH \
    (CACHE_LINE_SIZE_IN_BYTES - sizeof(std::atomic<int64_t>))/8
#define SEQUENCE_PADDING_LENGTH \
    (CACHE_LINE_SIZE_IN_BYTES - sizeof(int64_t))/8

#ifndef DISRUPTOR_SEQUENCE_H_ // NOLINT
#define DISRUPTOR_SEQUENCE_H_ // NOLINT

#include <atomic>

#include "disruptor/utils.h"

namespace disruptor {

const int64_t kInitialCursorValue = -1L;

// Sequence counter.
class Sequence {
 public:
    // Construct a sequence counter that can be tracked across threads.
    //
    // @param initial_value for the counter.
    Sequence(int64_t initial_value = kInitialCursorValue) :
            value_(initial_value) {}

    virtual ~Sequence() {}

    // Get the current value of the {@link Sequence}.
    //
    // @return the current value.
    int64_t sequence() const { return value_.load(std::memory_order::memory_order_acquire); }

    // Set the current value of the {@link Sequence}.
    //
    // @param the value to which the {@link Sequence} will be set.
    void set_sequence(int64_t value) { value_.store(value, std::memory_order::memory_order_release); }

    // Increment and return the value of the {@link Sequence}.
    //
    // @param increment the {@link Sequence}.
    // @return the new value incremented.
    int64_t IncrementAndGet(const int64_t& increment) {
        return value_.fetch_add(increment, std::memory_order::memory_order_release) + increment;
    }

 private:
    // members
    std::atomic<int64_t> value_;

    DISALLOW_COPY_AND_ASSIGN(Sequence);
};

// Cache line padded sequence counter.
//
// Can be used across threads without worrying about false sharing if a
// located adjacent to another counter in memory.
class PaddedSequence : public Sequence {
 public:
    PaddedSequence(int64_t initial_value = kInitialCursorValue) :
            Sequence(initial_value) {}

    virtual ~PaddedSequence() {}

 private:
    // padding
    int64_t padding_[ATOMIC_SEQUENCE_PADDING_LENGTH];

    DISALLOW_COPY_AND_ASSIGN(PaddedSequence);
};

// Non-atomic sequence counter.
//
// This counter is not thread safe.
class MutableLong {
 public:
     MutableLong(int64_t initial_value = kInitialCursorValue) :
         sequence_(initial_value) {}

     virtual ~MutableLong() {}

     int64_t sequence() const { return sequence_; }

     void set_sequence(const int64_t& sequence) { sequence_ = sequence; };

     int64_t IncrementAndGet(const int64_t& delta) { sequence_ += delta; return sequence_; }

 private:
     volatile int64_t sequence_;
     DISALLOW_COPY_AND_ASSIGN(MutableLong);
};

// Cache line padded non-atomic sequence counter.
//
// This counter is not thread safe.
class PaddedLong : public MutableLong {
 public:
     PaddedLong(int64_t initial_value = kInitialCursorValue) :
         MutableLong(initial_value) {}

     virtual ~PaddedLong() {}

 private:
     int64_t padding_[SEQUENCE_PADDING_LENGTH];
     DISALLOW_COPY_AND_ASSIGN(PaddedLong);
};

int64_t GetMinimumSequence(
        const std::vector<Sequence*>& sequences) {
        int64_t minimum = LONG_MAX;

        for (Sequence* sequence_: sequences) {
            int64_t sequence = sequence_->sequence();
            minimum = minimum < sequence ? minimum : sequence;
        }

        return minimum;
};

};  // namespace disruptor

#endif // DISRUPTOR_SEQUENCE_H_ NOLINT
