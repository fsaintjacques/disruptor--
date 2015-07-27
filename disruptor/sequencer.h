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

#ifndef DISRUPTOR_SEQUENCER_H_  // NOLINT
#define DISRUPTOR_SEQUENCER_H_  // NOLINT

#include "disruptor/claim_strategy.h"
#include "disruptor/wait_strategy.h"
#include "disruptor/sequence_barrier.h"

namespace disruptor {

// Coordinator for claiming sequences for access to a data structures while
// tracking dependent {@link Sequence}s
template <typename T, size_t N = kDefaultRingBufferSize,
          typename C = kDefaultClaimStrategy, typename W = kDefaultWaitStrategy>
class Sequencer {
 public:
  // Construct a Sequencer with the selected strategies.
  Sequencer(std::array<T, N> events) : ring_buffer_(events) {}

  // Set the sequences that will gate publishers to prevent the buffer
  // wrapping.
  //
  // @param sequences to be gated on.
  void set_gating_sequences(const std::vector<Sequence*>& sequences) {
    gating_sequences_ = sequences;
  }

  // Create a {@link SequenceBarrier} that gates on the cursor and a list of
  // {@link Sequence}s.
  //
  // @param sequences_to_track this barrier will track.
  // @return the barrier gated as required.
  SequenceBarrier<W> NewBarrier(const std::vector<Sequence*>& dependents) {
    return SequenceBarrier<W>(cursor_, dependents);
  }

  // Get the value of the cursor indicating the published sequence.
  //
  // @return value of the cursor for events that have been published.
  int64_t GetCursor() { return cursor_.sequence(); }

  // Has the buffer capacity left to allocate another sequence. This is a
  // concurrent method so the response should only be taken as an indication
  // of available capacity.
  //
  // @return true if the buffer has the capacity to allocated another event.
  bool HasAvailableCapacity() {
    return claim_strategy_.HasAvailableCapacity(gating_sequences_);
  }

  // Claim the next batch of sequence numbers for publishing.
  //
  // @param delta  the requested number of sequences.
  // @return the maximal claimed sequence
  int64_t Claim(size_t delta = 1) {
    return claim_strategy_.IncrementAndGet(gating_sequences_, delta);
  }

  // Publish an event and make it visible to {@link EventProcessor}s.
  //
  // @param sequence to be published.
  void Publish(const int64_t& sequence, size_t delta = 1) {
    claim_strategy_.SynchronizePublishing(sequence, cursor_, delta);
    const int64_t new_cursor = cursor_.IncrementAndGet(delta);
    wait_strategy_.SignalAllWhenBlocking();
  }

  T& operator[](const int64_t& sequence) { return ring_buffer_[sequence]; }

 private:
  // Members
  RingBuffer<T, N> ring_buffer_;

  Sequence cursor_;

  C claim_strategy_;

  W wait_strategy_;

  std::vector<Sequence*> gating_sequences_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(Sequencer);
};

};  // namespace disruptor

#endif  // DISRUPTOR_RING_BUFFER_H_ NOLINT
