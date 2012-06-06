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

#ifndef DISRUPTOR_WAITSTRATEGY_H_  // NOLINT
#define DISRUPTOR_WAITSTRATEGY_H_  // NOLINT

#include <sys/time.h>

#include <chrono>
#include <thread>
#include <vector>  

#include <mutex>
#include <condition_variable>

#include "disruptor/exceptions.h"
#include "disruptor/interface.h"
#include "disruptor/sequence.h"

namespace disruptor {

// Strategy options which are available to those waiting on a
// {@link RingBuffer}
enum WaitStrategyOption {
    // This strategy uses a condition variable inside a lock to block the
    // event procesor which saves CPU resource at the expense of lock
    // contention.
    kBlockingStrategy,
    // This strategy uses a progressive back off strategy by first spinning,
    // then yielding, then sleeping for 1ms period. This is a good strategy
    // for burst traffic then quiet periods when latency is not critical.
    kSleepingStrategy,
    // This strategy calls Thread.yield() in a loop as a waiting strategy
    // which reduces contention at the expense of CPU resource.
    kYieldingStrategy,
    // This strategy call spins in a loop as a waiting strategy which is
    // lowest and most consistent latency but ties up a CPU.
    kBusySpinStrategy
};

// Blocking strategy that uses a lock and condition variable for
// {@link Consumer}s waiting on a barrier.
// This strategy should be used when performance and low-latency are not as
// important as CPU resource.
class BlockingStrategy :  public WaitStrategyInterface {
 public:
    BlockingStrategy() {}

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        // We need to wait.
        if ((available_sequence = cursor.sequence()) < sequence) {
            // acquire lock
            std::unique_lock<std::recursive_mutex> ulock(mutex_);
            while ((available_sequence = cursor.sequence()) < sequence) {
                barrier.CheckAlert();
                consumer_notify_condition_.wait(ulock);
            }
        } // unlock happens here, on ulock destruction.

        if (0 != dependents.size()) {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                barrier.CheckAlert();
            }
        }

        return available_sequence;
    }

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence,
                            const int64_t& timeout_micros) {
        int64_t available_sequence = 0;
        // We have to wait
        if ((available_sequence = cursor.sequence()) < sequence) {
            std::unique_lock<std::recursive_mutex> ulock(mutex_);
            while ((available_sequence = cursor.sequence()) < sequence) {
                barrier.CheckAlert();
                if (std::cv_status::timeout == consumer_notify_condition_.wait_for(ulock,
                    std::chrono::microseconds(timeout_micros)))
                    break;

            }
        } // unlock happens here, on ulock destruction

        if (0 != dependents.size()) {
            while ((available_sequence = GetMinimumSequence(dependents)) \
                    < sequence) {
                barrier.CheckAlert();
            }
        }

        return available_sequence;
    }

    virtual void SignalAllWhenBlocking() {
        std::unique_lock<std::recursive_mutex> ulock(mutex_);
        consumer_notify_condition_.notify_all();
    }

 private:
    std::recursive_mutex mutex_;
    std::condition_variable_any consumer_notify_condition_;

    DISALLOW_COPY_AND_ASSIGN(BlockingStrategy);
};

// Sleeping strategy
class SleepingStrategy :  public WaitStrategyInterface {
 public:
    SleepingStrategy() {}

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        int counter = kRetries;

        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                counter = ApplyWaitMethod(barrier, counter);
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                counter = ApplyWaitMethod(barrier, counter);
            }
        }

        return available_sequence;
    }

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence,
                            const int64_t& timeout_micros) {
        // timing
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        int64_t start_micro = start_time.tv_sec*1000000 + start_time.tv_usec;

        int64_t available_sequence = 0;
        int counter = kRetries;

        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                counter = ApplyWaitMethod(barrier, counter);
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                counter = ApplyWaitMethod(barrier, counter);
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        }

        return available_sequence;
    }

    virtual void SignalAllWhenBlocking() {}

    static const int kRetries = 200;

 private:
    int ApplyWaitMethod(const SequenceBarrierInterface& barrier, int counter) {
        barrier.CheckAlert();
        if (counter > 100) {
            counter--;
        } else if (counter > 0) {
            counter--;
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return counter;
    }

    DISALLOW_COPY_AND_ASSIGN(SleepingStrategy);
};

// Yielding strategy that uses a sleep(0) for {@link EventProcessor}s waiting
// on a barrier. This strategy is a good compromise between performance and
// CPU resource.
class YieldingStrategy :  public WaitStrategyInterface {
 public:
    YieldingStrategy() {}

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        int counter = kSpinTries;

        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                counter = ApplyWaitMethod(barrier, counter);
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                counter = ApplyWaitMethod(barrier, counter);
            }
        }

        return available_sequence;
    }

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence,
                            const int64_t& timeout_micros) {
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        int64_t start_micro = start_time.tv_sec*1000000 + start_time.tv_usec;

        int64_t available_sequence = 0;
        int counter = kSpinTries;

        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                counter = ApplyWaitMethod(barrier, counter);
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                counter = ApplyWaitMethod(barrier, counter);
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        }

        return available_sequence;
    }

    virtual void SignalAllWhenBlocking() {}

    static const int kSpinTries = 100;

 private:
    int ApplyWaitMethod(const SequenceBarrierInterface& barrier, int counter) {
        barrier.CheckAlert();
        if (counter == 0) {
            std::this_thread::yield();
        } else {
            counter--;
        }

        return counter;
    }

    DISALLOW_COPY_AND_ASSIGN(YieldingStrategy);
};


// Busy Spin strategy that uses a busy spin loop for {@link EventProcessor}s
// waiting on a barrier.
// This strategy will use CPU resource to avoid syscalls which can introduce
// latency jitter.  It is best used when threads can be bound to specific
// CPU cores.
class BusySpinStrategy :  public WaitStrategyInterface {
 public:
    BusySpinStrategy() {}

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                barrier.CheckAlert();
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                barrier.CheckAlert();
            }
        }

        return available_sequence;
    }

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence,
                            const int64_t& timeout_micros) {
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        int64_t start_micro = start_time.tv_sec*1000000 + start_time.tv_usec;
        int64_t available_sequence = 0;

        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                barrier.CheckAlert();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                barrier.CheckAlert();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        }

        return available_sequence;
    }

    virtual void SignalAllWhenBlocking() {}

    DISALLOW_COPY_AND_ASSIGN(BusySpinStrategy);
};

WaitStrategyInterface* CreateWaitStrategy(WaitStrategyOption wait_option) {
    switch (wait_option) {
        case kBlockingStrategy:
            return new BlockingStrategy();
        case kSleepingStrategy:
            return new SleepingStrategy();
        case kYieldingStrategy:
            return new YieldingStrategy();
        case kBusySpinStrategy:
            return new BusySpinStrategy();
        default:
            return NULL;
    }
}


};  // namespace disruptor

#endif // DISRUPTOR_WAITSTRATEGY_H_  NOLINT
