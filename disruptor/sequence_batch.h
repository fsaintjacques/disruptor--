// Copyright 2011 <François Saint-Jacques>

#ifndef DISRUPTOR_SEQUENCE_BATCH_H_  // NOLINT
#define DISRUPTOR_SEQUENCE_BATCH_H_  // NOLINT

namespace disruptor {

class SequenceBatch {
 public:
    SequenceBatch(int size) : size_(size) {}

    int get_size() const { return size_; }

    void set_end(int64_t end) { end_ = end; }

    int64_t get_end() const { return end_; }

    int64_t get_start() const { return end_ - size_ + 1L; }

 private:
    int size_;
    int64_t end_;
};

};  // namespace disruptor

#endif // DISRUPTOR_SEQUENCE_BATCH_H_  NOLINT
