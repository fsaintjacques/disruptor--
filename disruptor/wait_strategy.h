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

#ifndef DISRUPTOR_WAITSTRATEGY_H_  // NOLINT
#define DISRUPTOR_WAITSTRATEGY_H_  // NOLINT

#include <sys/time.h>

#include <chrono>
#include <thread>
#include <condition_variable>
#include <vector>

#include "disruptor/sequence.h"

namespace disruptor {

/*
// Strategy employed for a {@link Consumer} to wait on the sequencer's
// cursor and a set of consumers' {@link Sequence}s.
//
class WaitStrategy {
 public:
  // Wait for the given sequence to be available for consumption.
  //
  // @param sequence    to wait for.
  // @param cursor      sequencer's cursor.
  // @param dependents
  // @param alerted     indicator of consumer alert.
  //
  // @return kAltertedSignal if the barrier signaled an alert, otherwise
  //         return the greatest available sequence which may be greater
  //         than requested.
  int64_t WaitFor(const int64_t& sequence,
                  const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted);

  // Wait for the given sequence to be available for consumption with a
  // specified timeout.
  //
  // @param sequence    to wait for.
  // @param cursor      sequencer maximal sequence.
  // @param dependents  further back the chain that must advance first.
  // @param alerted     indicator of consumer state.
  // @param timeout     indicator of consumer state.
  //
  // @return kAltertedSignal if the barrier signaled an alert,
  //         kTimeoutSignal if the the requested timeout was reached while
  //         waiting, otherwise return the greatest available sequence which
  //         may be greater than requested.
  //
  int64_t WaitFor(const int64_t& sequence,
                  const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& consumer_is_running,
                  const std::chrono::duration& timeout);

  // Signal the strategy that the cursor as advanced. Some strategy depends
  // on this behaviour to unblock.
  void SignalAllWhenBlocking();
};
*/

// Busy Spin strategy that uses a busy spin loop waiting on a barrier.
// This strategy will use CPU resource to avoid syscalls which can introduce
// latency jitter. It is strongly recommended to pin threads on isolated
// CPU cores to minimize context switching and latency.
class BusySpinStrategy;

// Yielding strategy that uses a thread::yield() for  waiting on a barrier.
// This strategy is a good compromise between performance and CPU resource.
template <int64_t S>
class YieldingStrategy;

// Sleeping strategy uses a progressive back off strategy by first spinning for
// S/2 loops, then yielding for S/2 loops, and finally sleeping for
// duration<D,DV> until ready to advance. This is a good strategy for burst
// traffic then quiet periods when latency is not critical.
template <int64_t S, typename D, int DV>
class SleepingStrategy;

// Blocking strategy that waits for the sequencer's cursor to advance on the
// requested sequence.
//
// The sequencer MUST call SignalAllWhenBlocking() to unblock the strategy
// when waiting on the cursor.  Once the cursor is advanced, the strategy will
// busy spin on the dependents' sequences and can be cancelled by affecting the
// `alerted` atomic.
//
// The user can optionnaly provide a maximum timeout to the blocking operation,
// see std::condition_any::wait_for() documentation for limitations.
//
// This strategy uses a condition variable inside a lock to block the
// event procesor which saves CPU resource at the expense of lock
// contention. Publishers must explicitely call SignalAllWhenBlocking()
// to unblock consumers. This strategy should be used when performance and
// low-latency are not as important as CPU resource.
class BlockingStrategy;

// defaults
using kDefaultWaitStrategy = BusySpinStrategy;
constexpr int64_t kDefaultRetryLoops = 200L;
using kDefaultDuration = std::chrono::milliseconds;
constexpr int kDefaultDurationValue = 1;

// used internally
static inline std::function<int64_t()> buildMinSequenceFunction(
    const Sequence& cursor, const std::vector<Sequence*>& dependents);

class BusySpinStrategy {
 public:
  BusySpinStrategy() {}

  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted) {
    int64_t available_sequence = kInitialCursorValue;
    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;
    }

    return available_sequence;
  }

  template <class R, class P>
  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted,
                  const std::chrono::duration<R, P>& timeout) {
    int64_t available_sequence = kInitialCursorValue;

    const auto start = std::chrono::system_clock::now();
    const auto stop = start + timeout;
    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;

      if (stop <= std::chrono::system_clock::now()) return kTimeoutSignal;
    }

    return available_sequence;
  }

  virtual void SignalAllWhenBlocking() {}

  DISALLOW_COPY_MOVE_AND_ASSIGN(BusySpinStrategy);
};

template <int64_t S = kDefaultRetryLoops>
class YieldingStrategy {
 public:
  YieldingStrategy() {}

  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted) {
    int64_t available_sequence = kInitialCursorValue;
    int counter = S;

    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;

      counter = ApplyWaitMethod(counter);
    }

    return available_sequence;
  }

  template <class R, class P>
  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted,
                  const std::chrono::duration<R, P>& timeout) {
    int64_t available_sequence = kInitialCursorValue;
    int64_t counter = S;

    const auto start = std::chrono::system_clock::now();
    const auto stop = start + timeout;
    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;

      counter = ApplyWaitMethod(counter);

      if (stop <= std::chrono::system_clock::now()) return kTimeoutSignal;
    }

    return available_sequence;
  }

  virtual void SignalAllWhenBlocking() {}

 private:
  inline int64_t ApplyWaitMethod(int64_t counter) {
    if (counter) {
      return --counter;
    }

    std::this_thread::yield();
    return counter;
  }

  DISALLOW_COPY_MOVE_AND_ASSIGN(YieldingStrategy);
};

template <int64_t S = kDefaultRetryLoops, typename D = kDefaultDuration,
          int DV = kDefaultDurationValue>
class SleepingStrategy {
 public:
  SleepingStrategy() {}

  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted) {
    int64_t available_sequence = kInitialCursorValue;
    int counter = S;

    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;

      counter = ApplyWaitMethod(counter);
    }

    return available_sequence;
  }

  template <class R, class P>
  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted,
                  const std::chrono::duration<R, P>& timeout) {
    int64_t available_sequence = kInitialCursorValue;
    int64_t counter = S;

    const auto start = std::chrono::system_clock::now();
    const auto stop = start + timeout;
    const auto min_sequence = buildMinSequenceFunction(cursor, dependents);

    while ((available_sequence = min_sequence()) < sequence) {
      if (alerted.load()) return kAlertedSignal;

      counter = ApplyWaitMethod(counter);

      if (stop <= std::chrono::system_clock::now()) return kTimeoutSignal;
    }

    return available_sequence;
  }

  void SignalAllWhenBlocking() {}

 private:
  inline int64_t ApplyWaitMethod(int64_t counter) {
    if (counter > (S / 2)) {
      --counter;
    } else if (counter > 0) {
      --counter;
      std::this_thread::yield();
    } else {
      std::this_thread::sleep_for(D(DV));
    }

    return counter;
  }

  DISALLOW_COPY_MOVE_AND_ASSIGN(SleepingStrategy);
};

class BlockingStrategy {
 public:
  BlockingStrategy() {}

  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted) {
    return WaitFor(sequence, cursor, dependents, alerted, [this](Lock& lock) {
      consumer_notify_condition_.wait(lock);
      return false;
    });
  }

  template <class Rep, class Period>
  int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                  const std::vector<Sequence*>& dependents,
                  const std::atomic<bool>& alerted,
                  const std::chrono::duration<Rep, Period>& timeout) {
    return WaitFor(sequence, cursor, dependents, alerted,
                   [this, timeout](Lock& lock) {
                     return std::cv_status::timeout ==
                            consumer_notify_condition_.wait_for(
                                lock, std::chrono::microseconds(timeout));
                   });
  }

  void SignalAllWhenBlocking() {
    std::unique_lock<std::recursive_mutex> ulock(mutex_);
    consumer_notify_condition_.notify_all();
  }

 private:
  using Lock = std::unique_lock<std::recursive_mutex>;
  using Waiter = std::function<bool(Lock&)>;

  inline int64_t WaitFor(const int64_t& sequence, const Sequence& cursor,
                         const std::vector<Sequence*>& dependents,
                         const std::atomic<bool>& alerted,
                         const Waiter& locker) {
    int64_t available_sequence = kInitialCursorValue;
    // BlockingStrategy is a special case where the unblock signal comes from
    // the sequencer. This is why we need to wait on the cursor first, and
    // then on the dependents.
    if ((available_sequence = cursor.sequence()) < sequence) {
      std::unique_lock<std::recursive_mutex> ulock(mutex_);
      while ((available_sequence = cursor.sequence()) < sequence) {
        if (alerted) return kAlertedSignal;

        // locker indicate if a timeout occured
        if (locker(ulock)) return kTimeoutSignal;
      }
    }

    // Now we wait on dependents.
    if (dependents.size()) {
      while ((available_sequence = GetMinimumSequence(dependents)) < sequence) {
        if (alerted) return kAlertedSignal;
      }
    }

    return available_sequence;
  }

  // members
  std::recursive_mutex mutex_;
  std::condition_variable_any consumer_notify_condition_;

  DISALLOW_COPY_MOVE_AND_ASSIGN(BlockingStrategy);
};

static inline std::function<int64_t()> buildMinSequenceFunction(
    const Sequence& cursor, const std::vector<Sequence*>& dependents) {
  if (!dependents.size())
    return [&cursor]() { return cursor.sequence(); };
  else
    return [&dependents]() { return GetMinimumSequence(dependents); };
}

};  // namespace disruptor

#endif  // DISRUPTOR_WAITSTRATEGY_H_  NOLINT
