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

#ifndef DISRUPTOR_RING_BUFFER_H_  // NOLINT
#define DISRUPTOR_RING_BUFFER_H_  // NOLINT

#include <array>
#include "utils.h"

namespace disruptor {

constexpr size_t kDefaultRingBufferSize = 1024;

// Ring buffer implemented with a fixed array.
//
// @param <T> event type
// @param <N> size of the ring
template <typename T, size_t N = kDefaultRingBufferSize>
class RingBuffer {
 public:
  // Construct a RingBuffer with the full option set.
  //
  // @param event_factory to instance new entries for filling the RingBuffer.
  // @param buffer_size of the RingBuffer, must be a power of 2.
  // @param claim_strategy_option threading strategy for publishers claiming
  // entries in the ring.
  // @param wait_strategy_option waiting strategy employed by
  // processors_to_track waiting in entries becoming available.
  RingBuffer(const std::array<T, N>& events) : events_(events) {}

  static_assert(((N > 0) && ((N & (~N + 1)) == N)),
                "RingBuffer's size must be a positive power of 2");

  // Get the event for a given sequence in the RingBuffer.
  //
  // @param sequence for the event
  // @return event reference at the specified sequence position.
  T& operator[](const int64_t& sequence) { return events_[sequence & (N - 1)]; }

  const T& operator[](const int64_t& sequence) const {
    return events_[sequence & (N - 1)];
  }

 private:
  std::array<T, N> events_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(RingBuffer);
};

};  // namespace disruptor

#endif  // DISRUPTOR_RING_BUFFER_H_ NOLINT
