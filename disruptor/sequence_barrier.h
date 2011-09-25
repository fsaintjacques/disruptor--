// Copyright 2011 <François Saint-Jacques>

#include <memory>
#include <vector>

#include "disruptor/interface.h"

#ifndef DISRUPTOR_SEQUENCE_BARRIER_H_ // NOLINT
#define DISRUPTOR_SEQUENCE_BARRIER_H_ // NOLINT

namespace disruptor {

class ProcessingSequenceBarrier : SequenceBarrierInterface {
 public:
    ProcessingSequenceBarrier(WaitStrategyInterface* wait_strategy,
                              Sequence* sequence,
                              const std::vector<Sequence*>& sequences) :
        cursor_(sequence),
        wait_strategy_(wait_strategy),
        dependent_sequences_(sequences),
        alerted_(false) {
    }

    virtual int64_t WaitFor(const int64_t& sequence) {
        return wait_strategy_->WaitFor(dependent_sequences_, *cursor_, *this,
                                       sequence);
    }

    virtual int64_t WaitFor(const int64_t& sequence,
                            const int64_t& timeout_micros) {
        return wait_strategy_->WaitFor(dependent_sequences_, *cursor_, *this,
                                       sequence, timeout_micros);
    }

    virtual int64_t GetCursor() const {
        return cursor_->sequence();
    }

    virtual bool IsAlerted() const {
        return alerted_.load();
    }

    virtual void Alert() {
        alerted_.store(true);
    }

    virtual void ClearAlert() {
        alerted_.store(false);
    }

 private:
    WaitStrategyInterface* wait_strategy_;
    Sequence* cursor_;
    std::vector<Sequence*> dependent_sequences_;
    std::atomic<bool> alerted_;
};

};  // namespace disruptor

#endif // DISRUPTOR_DEPENDENCY_BARRIER_H_ NOLINT
