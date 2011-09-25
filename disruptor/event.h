// Copyright 2011 <François Saint-Jacques>
#include <disruptor/sequence.h>

#ifndef CACHE_LINE_SIZE_IN_BYTES // NOLINT
#define CACHE_LINE_SIZE_IN_BYTES 64 // NOLINT
#endif // NOLINT
#define EVENT_PADDING_LENGTH \
    (CACHE_LINE_SIZE_IN_BYTES - sizeof(int64_t) - sizeof(T*))/8 // NOLINT

#ifndef DISRUPTOR_EVENT_H_ // NOLINT
#define DISRUPTOR_EVENT_H_ // NOLINT

namespace disruptor {

template <typename T>
class Event {
 public:
    Event() : sequence_(kInitialCursorValue) { }

    int64_t sequence() const { return sequence_; }

    void set_sequence(int64_t sequence) { sequence_ = sequence; }

    T* data() { return data_; }

    T* set_data(T* data) {
        T* tmp = data_;
        data_ = data;
        return tmp;
    }

 private:
    // members
    int64_t sequence_;
    T* data_;

    // padding
    int64_t padding_[EVENT_PADDING_LENGTH];

    DISALLOW_COPY_AND_ASSIGN(Event);
};

};  // namespace throughput

#endif // DISRUPTOR_EVENT_H_ NOLINT
