// Copyright 2011 <François Saint-Jacques>

#ifndef DISRUPTOR_BATCH_DESCRIPTOR_H_  // NOLINT
#define DISRUPTOR_BATCH_DESCRIPTOR_H_  // NOLINT

namespace disruptor {

class BatchDescriptor {
 public:
    BatchDescriptor(int size) :
        size_(size),
        end_(kInitialCursorValue) {}

    int size() const { return size_; }

    void set_end(int64_t end) { end_ = end; }

    int64_t end() const { return end_; }

    int64_t Start() const { return end_ - size_ + 1L; }

 private:
    int size_;
    int64_t end_;
};

};  // namespace disruptor

#endif // DISRUPTOR_SEQUENCE_BATCH_H_  NOLINT
