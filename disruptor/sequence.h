// Copyright 2011 <François Saint-Jacques>

#include <atomic>

#include "disruptor/utils.h"

#ifndef CACHE_LINE_SIZE_IN_BYTES // NOLINT
#define CACHE_LINE_SIZE_IN_BYTES 64 // NOLINT
#endif // NOLINT
#define SEQUENCE_PADDING_LENGTH \
    (CACHE_LINE_SIZE_IN_BYTES - sizeof(std::atomic<int64_t>))/8

#ifndef DISRUPTOR_SEQUENCE_H_ // NOLINT
#define DISRUPTOR_SEQUENCE_H_ // NOLINT

namespace disruptor {

const int64_t kInitialCursorValue = -1L;

class Sequence {
 public:
    Sequence(int64_t initial_value = kInitialCursorValue) :
            value_(initial_value) {}

    int64_t sequence() const { return value_; }

    void set_sequence(int64_t value) { value_.store(value); }

    int64_t IncrementAndGet(const int64_t& increment) {
        return value_.fetch_add(increment) + increment;
    }

 private:
    // members
    std::atomic<int64_t> value_;

    // padding
    int64_t padding_[SEQUENCE_PADDING_LENGTH];

    DISALLOW_COPY_AND_ASSIGN(Sequence);
};

};  // namespace throughput

#endif // DISRUPTOR_SEQUENCE_H_ NOLINT
