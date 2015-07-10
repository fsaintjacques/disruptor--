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

#include <disruptor/wait_strategy.h>

namespace disruptor {
namespace test {

template <typename W>
struct StrategyFixture {
  StrategyFixture() : alerted(false) {}

  Sequence cursor;
  Sequence sequence_1;
  Sequence sequence_2;
  Sequence sequence_3;
  std::vector<Sequence*> dependents;
  W strategy;
  std::atomic<bool> alerted;

  std::vector<Sequence*> allDependents() {
    std::vector<Sequence*> d = {&sequence_1, &sequence_2, &sequence_3};
    return d;
  }
};

/* BusySpingStrategy */
using BusySpinStrategyFixture = StrategyFixture<BusySpinStrategy>;
BOOST_FIXTURE_TEST_SUITE(BusySpinStrategy, BusySpinStrategyFixture)

BOOST_AUTO_TEST_CASE(WaitForCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(
        strategy.WaitFor(kFirstSequenceValue, cursor, dependents, alerted));
  });

  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);
  std::thread([this]() {
    cursor.IncrementAndGet(1L);
    strategy.SignalAllWhenBlocking();
  }).join();
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalTimeoutWaitingOnCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted,
                                        std::chrono::microseconds(1L)));
  });

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kTimeoutSignal);

  std::thread waiter2([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted, std::chrono::seconds(1L)));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  waiter2.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(WaitForDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_3.IncrementAndGet(1L);
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalAlertWaitingOnDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  alerted.store(true);

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kAlertedSignal);
}

BOOST_AUTO_TEST_SUITE_END()  // BusySpinStrategy suite

/* YieldingStrategy */
using YieldingStrategyFixture = StrategyFixture<YieldingStrategy<>>;
BOOST_FIXTURE_TEST_SUITE(YieldingStrategy, YieldingStrategyFixture)

BOOST_AUTO_TEST_CASE(WaitForCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(
        strategy.WaitFor(kFirstSequenceValue, cursor, dependents, alerted));
  });

  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);
  std::thread([this]() {
    cursor.IncrementAndGet(1L);
    strategy.SignalAllWhenBlocking();
  }).join();
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalTimeoutWaitingOnCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted,
                                        std::chrono::microseconds(1L)));
  });

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kTimeoutSignal);

  std::thread waiter2([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted, std::chrono::seconds(1L)));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  waiter2.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(WaitForDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_3.IncrementAndGet(1L);
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalAlertWaitingOnDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  alerted.store(true);

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kAlertedSignal);
}

BOOST_AUTO_TEST_SUITE_END()  // YieldingStrategy suite

/* SleepingStrategy */
using SleepingStrategyFixture = StrategyFixture<SleepingStrategy<>>;
BOOST_FIXTURE_TEST_SUITE(SleepingStrategy, SleepingStrategyFixture)

BOOST_AUTO_TEST_CASE(WaitForCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(
        strategy.WaitFor(kFirstSequenceValue, cursor, dependents, alerted));
  });

  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);
  std::thread([this]() {
    cursor.IncrementAndGet(1L);
    strategy.SignalAllWhenBlocking();
  }).join();
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalTimeoutWaitingOnCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted,
                                        std::chrono::microseconds(1L)));
  });

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kTimeoutSignal);

  std::thread waiter2([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted, std::chrono::seconds(1L)));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  waiter2.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(WaitForDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_3.IncrementAndGet(1L);
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalAlertWaitingOnDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  alerted.store(true);

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kAlertedSignal);
}

BOOST_AUTO_TEST_SUITE_END()  // SleepingStrategy suite

/* BlockingStrategy */
using BlockingStrategyFixture = StrategyFixture<BlockingStrategy>;
BOOST_FIXTURE_TEST_SUITE(BlockingStrategy, BlockingStrategyFixture)

BOOST_AUTO_TEST_CASE(WaitForCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(
        strategy.WaitFor(kFirstSequenceValue, cursor, dependents, alerted));
  });

  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);
  std::thread([this]() {
    cursor.IncrementAndGet(1L);
    strategy.SignalAllWhenBlocking();
  }).join();
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalAlertWaitingOnCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(
        strategy.WaitFor(kFirstSequenceValue, cursor, dependents, alerted));
  });

  std::thread([this]() { strategy.SignalAllWhenBlocking(); }).join();
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  std::thread([this]() {
    alerted.store(true);
    strategy.SignalAllWhenBlocking();
  }).join();

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kAlertedSignal);
}

BOOST_AUTO_TEST_CASE(SignalTimeoutWaitingOnCursor) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted,
                                        std::chrono::microseconds(1L)));
  });

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kTimeoutSignal);

  std::thread waiter2([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor, dependents,
                                        alerted, std::chrono::seconds(1L)));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  waiter2.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(WaitForDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_3.IncrementAndGet(1L);
  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kFirstSequenceValue);
}

BOOST_AUTO_TEST_CASE(SignalAlertWaitingOnDependents) {
  std::atomic<int64_t> return_value(kInitialCursorValue);

  std::thread waiter([this, &return_value]() {
    return_value.store(strategy.WaitFor(kFirstSequenceValue, cursor,
                                        allDependents(), alerted));
  });

  cursor.IncrementAndGet(1L);
  strategy.SignalAllWhenBlocking();
  // dependents haven't moved, WaitFor() should still block.
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_1.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  sequence_2.IncrementAndGet(1L);
  BOOST_CHECK_EQUAL(return_value.load(), kInitialCursorValue);

  alerted.store(true);

  waiter.join();
  BOOST_CHECK_EQUAL(return_value.load(), kAlertedSignal);
}

BOOST_AUTO_TEST_SUITE_END()  // BlockingStrategy suite

};  // namespace test
};  // namespace disruptor
