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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE WaitStrategyTest

#include <boost/test/unit_test.hpp>
#include <disruptor/claim_strategy.h>

#define RING_BUFFER_SIZE 8

namespace disruptor {
namespace test {

struct SingleThreadedStrategyFixture {
  Sequence cursor;
  Sequence sequence_1;
  Sequence sequence_2;
  Sequence sequence_3;
  std::vector<Sequence*> empty_dependents;
  SingleThreadedStrategy<RING_BUFFER_SIZE> strategy;

  std::vector<Sequence*> oneDependents() {
    std::vector<Sequence*> d = {&sequence_1};
    return d;
  }

  std::vector<Sequence*> allDependents() {
    std::vector<Sequence*> d = {&sequence_1, &sequence_2, &sequence_3};
    return d;
  }
};

const int64_t kFirstSequenceValue = kInitialCursorValue + 1L;

BOOST_AUTO_TEST_SUITE(SingleThreadedStrategy)

BOOST_FIXTURE_TEST_CASE(IncrementAndGet, SingleThreadedStrategyFixture) {
  int64_t return_value = strategy.IncrementAndGet(empty_dependents);
  BOOST_CHECK_EQUAL(return_value, kFirstSequenceValue);

  const int delta = 10;
  return_value = strategy.IncrementAndGet(delta, empty_dependents);
  BOOST_CHECK_EQUAL(return_value, kFirstSequenceValue + delta);
}

BOOST_FIXTURE_TEST_CASE(HasAvalaibleCapacity, SingleThreadedStrategyFixture) {
  auto one_dependents = oneDependents();

  int64_t return_value =
      strategy.IncrementAndGet(RING_BUFFER_SIZE, one_dependents);
  BOOST_CHECK_EQUAL(return_value, kInitialCursorValue + RING_BUFFER_SIZE);
  BOOST_CHECK_EQUAL(strategy.HasAvalaibleCapacity(one_dependents), false);

  // advance late consumers
  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(strategy.HasAvalaibleCapacity(one_dependents), true);

  // only one slot free
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(one_dependents),
                    return_value + 1L);

  // dependent keeps up
  sequence_1.IncrementAndGet(RING_BUFFER_SIZE);

  // all equals
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(RING_BUFFER_SIZE, one_dependents),
                    sequence_1.IncrementAndGet(RING_BUFFER_SIZE));
}

BOOST_FIXTURE_TEST_CASE(SetSequence, SingleThreadedStrategyFixture) {
  auto one_dependents = oneDependents();

  std::atomic<int64_t> return_value(kInitialCursorValue);
  std::thread waiter([this, &one_dependents]() {
    strategy.SetSequence(RING_BUFFER_SIZE, one_dependents);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // advance late consumers
  sequence_1.IncrementAndGet(2L);

  // all equals
  waiter.join();
  BOOST_CHECK_EQUAL(strategy.HasAvalaibleCapacity(one_dependents), true);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace test
};  // namespace disruptor
