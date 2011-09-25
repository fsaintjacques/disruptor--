// Copyright 2011 <François Saint-Jacques>

#include <sys/time.h>

#include <chrono>
#include <thread>
#include <vector>

#include "disruptor/exceptions.h"
#include "disruptor/interface.h"
#include "disruptor/sequence.h"

#ifndef DISRUPTOR_WAITSTRATEGY_H_  // NOLINT
#define DISRUPTOR_WAITSTRATEGY_H_  // NOLINT

namespace disruptor {

// Strategy options which are available to those waiting on a
// {@link RingBuffer}
enum WaitStrategyOption {
    // This strategy uses a condition variable inside a lock to block the
    // event procesor which saves CPU resource at the expense of lock
    // contention.
    kBlockingStrategy,
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
    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        if ((available_sequence = cursor.sequence()) < sequence) {
            std::unique_lock<std::recursive_mutex> ulock(mutex_);
            while ((available_sequence = cursor.sequence()) < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                consumer_notify_condition_.wait(ulock);
            }
        }

        if (0 != dependents.size()) {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
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
        if ((available_sequence = cursor.sequence()) < sequence) {
            std::unique_lock<std::recursive_mutex> ulock(mutex_);
            ulock.lock();
            while ((available_sequence = cursor.sequence()) < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();

                if (std::cv_status::timeout == consumer_notify_condition_.wait_for(ulock,
                    std::chrono::microseconds(timeout_micros)))
                    break;

            }
            // ulock.unlock();
        }

        if (0 != dependents.size()) {
            while ((available_sequence = GetMinimumSequence(dependents)) \
                    < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
            }
        }

        return available_sequence;
    }

    virtual void SignalAll() {
        consumer_notify_condition_.notify_all();
    }

 private:
    std::recursive_mutex mutex_;
    std::condition_variable_any consumer_notify_condition_;
};

// Yielding strategy that uses a sleep(0) for {@link EventProcessor}s waiting
// on a barrier. This strategy is a good compromise between performance and
// CPU resource.
class YieldingStrategy :  public WaitStrategyInterface {
 public:
    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                std::this_thread::yield();
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                std::this_thread::yield();
            }
        }

        return available_sequence;
    }

    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence,
                            const int64_t & timeout_micros) {
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        int64_t start_micro = start_time.tv_sec*1000000 + start_time.tv_usec;
        int64_t available_sequence = 0;
        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                std::this_thread::yield();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                std::this_thread::yield();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        }

        return available_sequence;
    }

    virtual void SignalAll() {}
};


// Busy Spin strategy that uses a busy spin loop for {@link EventProcessor}s
// waiting on a barrier.
// This strategy will use CPU resource to avoid syscalls which can introduce
// latency jitter.  It is best used when threads can be bound to specific
// CPU cores.
class BusySpinStrategy :  public WaitStrategyInterface {
 public:
    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) {
        int64_t available_sequence = 0;
        if (0 == dependents.size()) {
            while ((available_sequence = cursor.sequence()) < sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
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
                if (barrier.IsAlerted())
                    throw AlertException();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        } else {
            while ((available_sequence = GetMinimumSequence(dependents)) < \
                    sequence) {
                if (barrier.IsAlerted())
                    throw AlertException();
                gettimeofday(&end_time, NULL);
                int64_t end_micro = end_time.tv_sec*1000000 + end_time.tv_usec;
                if (timeout_micros < (end_micro - start_micro))
                    break;
            }
        }

        return available_sequence;
    }

    virtual void SignalAll() {}
};

WaitStrategyInterface* CreateWaitStrategy(WaitStrategyOption wait_option) {
    switch (wait_option) {
        case kBlockingStrategy:
            return new BlockingStrategy();
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
