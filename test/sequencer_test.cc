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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SequencerTest

#include <atomic>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include <disruptor/sequencer.h>

#define RING_BUFFER_SIZE 4

namespace disruptor {
namespace test {

struct SequencerFixture {
  SequencerFixture() : events({1L, 2L, 3L, 4L}), sequencer(events){};

  void FillBuffer() {
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
      int64_t sequence = sequencer.Claim();
      sequencer.Publish(sequence);
    }
  }

  Sequencer<long, RING_BUFFER_SIZE, SingleThreadedStrategy<RING_BUFFER_SIZE>,
            kDefaultWaitStrategy> sequencer;
  std::array<long, RING_BUFFER_SIZE> events;
};

BOOST_FIXTURE_TEST_SUITE(SequencerBasic, SequencerFixture)

BOOST_AUTO_TEST_CASE(ShouldStartWithValueInitialized) {
  BOOST_CHECK(sequencer.GetCursor() == kInitialCursorValue);
}

BOOST_AUTO_TEST_SUITE_END()  // BlockingStrategy suite

};  // namepspace test
};  // namepspace disruptor
