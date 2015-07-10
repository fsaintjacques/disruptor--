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
#define BOOST_TEST_MODULE SequenceBarrier

#define RING_BUFFER_SIZE 8

#include <boost/test/unit_test.hpp>
#include <disruptor/sequence_barrier.h>

namespace disruptor {
namespace test {

struct SequenceBarrierFixture {
  SequenceBarrierFixture() : barrier(cursor, dependents) {}

  Sequence cursor;
  Sequence sequence_1;
  Sequence sequence_2;
  Sequence sequence_3;
  std::vector<Sequence*> dependents;
  SequenceBarrier<> barrier;

  std::vector<Sequence*> allDependents() {
    std::vector<Sequence*> d = {&sequence_1, &sequence_2, &sequence_3};
    return d;
  }
};

BOOST_FIXTURE_TEST_SUITE(SequenceBarrier, SequenceBarrierFixture)

BOOST_AUTO_TEST_CASE(BasicSetterAndGetter) {
  BOOST_CHECK_EQUAL(barrier.alerted(), false);
  barrier.set_alerted(true);
  BOOST_CHECK_EQUAL(barrier.alerted(), true);
}

BOOST_AUTO_TEST_CASE(WaitForCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(barrier.WaitFor(kFirstSequenceValue));
  });

  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);
  std::thread([this]() { cursor.IncrementAndGet(1L); }).join();
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);

  std::thread waiter2([this, &return_value]() {
    return_value.store(
        barrier.WaitFor(kFirstSequenceValue + 1L, std::chrono::seconds(5)));
  });

  std::thread([this]() { cursor.IncrementAndGet(1L); }).join();

  waiter2.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue + 1L);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace test
};  // namespace disruptor
