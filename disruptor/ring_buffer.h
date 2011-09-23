// Copyright 2011 <François Saint-Jacques>

#include <vector>

#include "disruptor/interface.h"
#include "disruptor/claim_strategy.h"
#include "disruptor/wait_strategy.h"
#include "disruptor/sequence_barrier.h"

#ifndef DISRUPTOR_RING_BUFFER_H_ // NOLINT
#define DISRUPTOR_RING_BUFFER_H_ // NOLINT

// #define buffer_size_ 256

namespace disruptor {

template<typename T>
class RingBuffer :  public PublisherPortInterface<T> {
 public:
    RingBuffer(ClaimStrategyOption claim_strategy_option,
               WaitStrategyOption wait_strategy_option,
               int buffer_size) :
        buffer_size_(buffer_size),
        mask_(buffer_size_ - 1),
        events_(new Event<T>[buffer_size_]),
        claim_strategy_(CreateClaimStrategy(claim_strategy_option,
                                            buffer_size_)),
        wait_strategy_(CreateWaitStrategy(wait_strategy_option)) {}

    ~RingBuffer() {
        delete events_;
    }

    ProcessingSequenceBarrier* SetTrackedProcessor(
            const std::vector<EventProcessorInterface<T>*>& event_processors) {
        std::vector<Sequence*> dependent_sequences;
        for (int i = 0; i < event_processors.size(); i++)
            dependent_sequences.push_back(event_processors[i]->GetSequence());

        return new ProcessingSequenceBarrier(wait_strategy_.get(), &cursor_,
                                             dependent_sequences);
    }


    int GetCapacity() { return buffer_size_; }

    virtual Event<T>* GetEvent(const int64_t& sequence) {
        return &events_[sequence % mask_];
    }

    virtual int64_t GetCursor() { return cursor_.sequence(); }

    virtual Event<T>* NextEvent() {
        int64_t sequence = claim_strategy_->IncrementAndGet();
        claim_strategy_->EnsureProcessorsAreInRange(sequence,
                processor_sequences_to_track_);

        Event<T>& event = events_[sequence % mask_];
        event.set_sequence(sequence);

        return &event;
    }

    SequenceBatch* NextEvents(SequenceBatch* sequence_batch) {
        long sequence = claim_strategy_->IncrementAndGet(
                sequence_batch->get_size());
        sequence_batch->set_end(sequence);
        claim_strategy_->EnsureProcessorsAreInRange(
                sequence, processor_sequences_to_track_);

        for (long i = sequence_batch->get_start(),
                end = sequence_batch->get_end(); i <= end; i++) {
            Event<T>& event = events_[i & mask_];
            event.set_sequence(i);
        }

        return sequence_batch;
    }

    void Publish(const long& sequence) {
        Publish(sequence, 1);
    }

    void Publish(const SequenceBatch& sequence_batch) {
        Publish(sequence_batch.get_end(), sequence_batch.get_size());
    }


 private:
    // Helpers
    void Publish(const int64_t& sequence, const int64_t& batch_size) {
        claim_strategy_->SerialisePublishing(cursor_, sequence, batch_size);
        cursor_.set_sequence(sequence);
        wait_strategy_->SignalAll();
    }

    // Members
    Sequence cursor_;
    int buffer_size_;
    int mask_;
    Event<T>* events_;
    std::vector<Sequence*> processor_sequences_to_track_;

    const boost::scoped_ptr<ClaimStrategyInterface> claim_strategy_;
    const boost::scoped_ptr<WaitStrategyInterface> wait_strategy_;
};

};  // namespace disruptor

#endif // DISRUPTOR_RING_BUFFER_H_ NOLINT
