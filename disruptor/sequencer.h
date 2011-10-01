// Copyright 2011 <François Saint-Jacques>

#include <vector>

#include "disruptor/batch_descriptor.h"
#include "disruptor/interface.h"
#include "disruptor/claim_strategy.h"
#include "disruptor/wait_strategy.h"
#include "disruptor/sequence_barrier.h"

#ifndef DISRUPTOR_SEQUENCER_H_ // NOLINT
#define DISRUPTOR_SEQUENCER_H_ // NOLINT

namespace disruptor {

class Sequencer {
 public:
    Sequencer(int buffer_size,
              ClaimStrategyOption claim_strategy_option,
              WaitStrategyOption wait_strategy_option) :
            buffer_size_(buffer_size),
            claim_strategy_(CreateClaimStrategy(claim_strategy_option,
                                                buffer_size_)),
            wait_strategy_(CreateWaitStrategy(wait_strategy_option)) { }

    ~Sequencer() {
        delete claim_strategy_;
        delete wait_strategy_;
    }

    void set_gating_sequences(
            const std::vector<Sequence*>& sequences) {
        gating_sequences_ = sequences;
    }

    int buffer_size() { return buffer_size_; }

    ProcessingSequenceBarrier* SetTrackedProcessor(
            const std::vector<Sequence*>& sequences_to_track) {
        return new ProcessingSequenceBarrier(wait_strategy_, &cursor_,
                                             sequences_to_track);
    }

    BatchDescriptor* NewBatchDescriptor(const int& size) {
        return new BatchDescriptor(size<buffer_size_?size:buffer_size_);
    }


    int64_t GetCursor() { return cursor_.sequence(); }

    bool HasAvalaibleCapacity() {
        claim_strategy_->HasAvalaibleCapacity(gating_sequences_);
    }

    int64_t Next() {
        return claim_strategy_->IncrementAndGet(gating_sequences_);
    }

    BatchDescriptor* Next(BatchDescriptor* batch_descriptor) {
        int64_t sequence = claim_strategy_->IncrementAndGet(batch_descriptor->size(), gating_sequences_);
        batch_descriptor->set_end(sequence);
        return batch_descriptor;
    }

    int64_t Claim(const int64_t& sequence) {
        claim_strategy_->SetSequence(sequence, gating_sequences_);
        return sequence;
    }

    void Publish(const int64_t& sequence) {
        Publish(sequence, 1);
    }

    void Publish(const BatchDescriptor& batch_descriptor) {
        Publish(batch_descriptor.end(), batch_descriptor.size());
    }

    void ForcePublish(const int64_t& sequence) {
        cursor_.set_sequence(sequence);
        wait_strategy_->SignalAllWhenBlocking();
    }


 private:
    // Helpers
    void Publish(const int64_t& sequence, const int64_t& batch_size) {
        claim_strategy_->SerialisePublishing(sequence, cursor_, batch_size);
        cursor_.set_sequence(sequence);
        wait_strategy_->SignalAllWhenBlocking();
    }

    // Members
    const int buffer_size_;

    Sequence cursor_;
    std::vector<Sequence*> gating_sequences_;

    ClaimStrategyInterface* claim_strategy_;
    WaitStrategyInterface* wait_strategy_;
};

};  // namespace disruptor

#endif // DISRUPTOR_RING_BUFFER_H_ NOLINT
