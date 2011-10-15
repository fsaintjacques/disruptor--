// Copyright 2011 <François Saint-Jacques>

#include "disruptor/sequence.h"

#ifndef DISRUPTOR_BATCH_DESCRIPTOR_H_  // NOLINT
#define DISRUPTOR_BATCH_DESCRIPTOR_H_  // NOLINT

namespace disruptor {

// Used to record the batch of sequences claimed via {@link Sequencer}.
class BatchDescriptor {
 public:
    // Create a holder for tracking a batch of claimed sequences in a
    // {@link Sequencer}
    //
    // @param size of the batch to claim.
    BatchDescriptor(int size) :
        size_(size),
        end_(kInitialCursorValue) {}

    // Get the size of the batch
    int size() const { return size_; }

    // Get the end sequence of a batch.
    //
    // @return the end sequence in the batch.
    int64_t end() const { return end_; }

    // Set the end sequence of a batch.
    //
    // @param end sequence in the batch.
    void set_end(int64_t end) { end_ = end; }


    // Get the starting sequence of the batch.
    //
    // @return starting sequence in the batch.
    int64_t Start() const { return end_ - size_ + 1L; }

 private:
    int size_;
    int64_t end_;
};

};  // namespace disruptor

#endif // DISRUPTOR_SEQUENCE_BATCH_H_  NOLINT
