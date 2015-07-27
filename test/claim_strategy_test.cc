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
#define BOOST_TEST_MODULE WaitStrategyTest

#include <boost/test/unit_test.hpp>
#include <disruptor/claim_strategy.h>

#define RING_BUFFER_SIZE 8

namespace disruptor {
namespace test {

template <typename S>
struct ClaimStrategyFixture {
  Sequence cursor;
  Sequence sequence_1;
  Sequence sequence_2;
  Sequence sequence_3;
  std::vector<Sequence*> empty_dependents;
  S strategy;

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

using SingleThreadedFixture =
    ClaimStrategyFixture<SingleThreadedStrategy<RING_BUFFER_SIZE>>;
BOOST_FIXTURE_TEST_SUITE(SingleThreadedStrategy, SingleThreadedFixture)

BOOST_AUTO_TEST_CASE(IncrementAndGet) {
  int64_t return_value = strategy.IncrementAndGet(empty_dependents);
  BOOST_CHECK_EQUAL(return_value, kFirstSequenceValue);

  const size_t delta = 10;
  return_value = strategy.IncrementAndGet(empty_dependents, delta);
  BOOST_CHECK_EQUAL(return_value, kFirstSequenceValue + delta);
}

BOOST_AUTO_TEST_CASE(HasAvailableCapacity) {
  auto one_dependents = oneDependents();

  int64_t return_value =
      strategy.IncrementAndGet(one_dependents, RING_BUFFER_SIZE);
  BOOST_CHECK_EQUAL(return_value, kInitialCursorValue + RING_BUFFER_SIZE);
  BOOST_CHECK_EQUAL(strategy.HasAvailableCapacity(one_dependents), false);

  // advance late consumers
  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(strategy.HasAvailableCapacity(one_dependents), true);

  // only one slot free
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(one_dependents),
                    return_value + 1L);

  // dependent keeps up
  sequence_1.IncrementAndGet(RING_BUFFER_SIZE);

  // all equals
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(one_dependents, RING_BUFFER_SIZE),
                    sequence_1.IncrementAndGet(RING_BUFFER_SIZE));
}

BOOST_AUTO_TEST_SUITE_END()

using MultiThreadedFixture =
    ClaimStrategyFixture<MultiThreadedStrategy<RING_BUFFER_SIZE>>;
BOOST_FIXTURE_TEST_SUITE(MultiThreadedStrategy, MultiThreadedFixture)

BOOST_AUTO_TEST_CASE(SingleIncrementAndGet) {
  std::atomic<int64_t> return_value(kInitialCursorValue);
  std::thread([this, &return_value]() {
    return_value.store(strategy.IncrementAndGet(empty_dependents));
  }).join();
  BOOST_CHECK_EQUAL(return_value, kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(DualIncrementAndGet) {
  std::atomic<int64_t> return_1, return_2 = {kInitialCursorValue};
  std::atomic<bool> wait_1, wait_2 = {true};

  std::thread publisher_1([this, &return_1, &wait_1]() {
    while (wait_1)
      ;
    return_1 = strategy.IncrementAndGet(empty_dependents);
  });

  std::thread publisher_2([this, &return_2, &wait_2]() {
    while (wait_2)
      ;
    return_2 = strategy.IncrementAndGet(empty_dependents);
  });

  wait_1 = false;
  publisher_1.join();

  wait_2 = false;
  publisher_2.join();

  BOOST_CHECK_EQUAL(return_1, kFirstSequenceValue);
  BOOST_CHECK_EQUAL(return_2, kFirstSequenceValue + 1L);
}

BOOST_AUTO_TEST_CASE(HasAvailableCapacity) {
  auto one_dependents = oneDependents();

  int64_t return_value =
      strategy.IncrementAndGet(one_dependents, RING_BUFFER_SIZE);
  BOOST_CHECK_EQUAL(return_value, kInitialCursorValue + RING_BUFFER_SIZE);
  BOOST_CHECK_EQUAL(strategy.HasAvailableCapacity(one_dependents), false);

  // advance late consumers
  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(strategy.HasAvailableCapacity(one_dependents), true);

  // only one slot free
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(one_dependents),
                    return_value + 1L);

  // dependent keeps up
  sequence_1.IncrementAndGet(RING_BUFFER_SIZE);

  // all equals
  BOOST_CHECK_EQUAL(strategy.IncrementAndGet(one_dependents, RING_BUFFER_SIZE),
                    sequence_1.IncrementAndGet(RING_BUFFER_SIZE));
}

BOOST_AUTO_TEST_CASE(SynchronizePublishingShouldBlockEagerThreads) {
  std::atomic<bool> running_1(true), running_2(true), running_3(true);
  std::atomic<bool> wait_1(true), wait_2(true), wait_3(true);
  Sequence claimed_1, claimed_2, claimed_3;
  Sequence cursor;

  std::thread publisher_1([this, &claimed_1, &cursor, &running_1, &wait_1]() {
    while (wait_1)
      ;
    claimed_1.set_sequence(strategy.IncrementAndGet(empty_dependents));
    wait_1 = true;
    while (wait_1)
      ;
    strategy.SynchronizePublishing(kFirstSequenceValue, cursor, 1);
    running_1 = false;
  });

  std::thread publisher_2([this, &claimed_2, &cursor, &running_2, &wait_2]() {
    while (wait_2)
      ;
    claimed_2.set_sequence(strategy.IncrementAndGet(empty_dependents));
    wait_2 = true;
    while (wait_2)
      ;
    strategy.SynchronizePublishing(kFirstSequenceValue + 1, cursor, 1);
    running_2 = false;
  });

  std::thread publisher_3([this, &claimed_3, &cursor, &running_3, &wait_3]() {
    while (wait_3)
      ;
    claimed_3.set_sequence(strategy.IncrementAndGet(empty_dependents));
    wait_3 = true;
    while (wait_3)
      ;
    strategy.SynchronizePublishing(kFirstSequenceValue + 2, cursor, 1);
    running_3 = false;
  });

  // publisher_1 claims
  wait_1 = false;
  while (!wait_1)
    ;
  BOOST_CHECK_EQUAL(claimed_1.sequence(), kFirstSequenceValue);
  BOOST_CHECK_EQUAL(claimed_2.sequence(), kInitialCursorValue);
  BOOST_CHECK_EQUAL(claimed_3.sequence(), kInitialCursorValue);

  // publisher_2 claims
  wait_2 = false;
  while (!wait_2)
    ;
  BOOST_CHECK_EQUAL(claimed_1.sequence(), kFirstSequenceValue);
  BOOST_CHECK_EQUAL(claimed_2.sequence(), kFirstSequenceValue + 1);
  BOOST_CHECK_EQUAL(claimed_3.sequence(), kInitialCursorValue);

  // publisher_3 claims
  wait_3 = false;
  while (!wait_3)
    ;
  BOOST_CHECK_EQUAL(claimed_1.sequence(), kFirstSequenceValue);
  BOOST_CHECK_EQUAL(claimed_2.sequence(), kFirstSequenceValue + 1);
  BOOST_CHECK_EQUAL(claimed_3.sequence(), kFirstSequenceValue + 2);

  // publisher 2 and 3 continue running but must wait on publisher 3 to
  // publish
  wait_3 = false;
  wait_2 = false;
  BOOST_CHECK(running_2);
  BOOST_CHECK(running_3);

  // publisher_1 publish his sequence
  wait_1 = false;
  publisher_1.join();
  BOOST_CHECK(running_2);
  BOOST_CHECK(running_3);

  // sequencer commit the cursor
  cursor.IncrementAndGet(1);

  // publisher_2 is now free to run
  publisher_2.join();
  // publisher_3 is still locked
  BOOST_CHECK(running_3);

  // sequencer commit the cursor once more
  cursor.IncrementAndGet(1);
  publisher_3.join();
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace test
};  // namespace disruptor
