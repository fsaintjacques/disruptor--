// Copyright 2011 <François Saint-Jacques>

#include "disruptor/ring_buffer.h"

#ifndef DISRUPTOR_EVENT_PUBLISHER_H_ // NOLINT
#define DISRUPTOR_EVENT_PUBLISHER_H_ // NOLINT

namespace disruptor {

template<typename T>
class EventPublisher {
 public:
    EventPublisher(RingBuffer<T>* ring_buffer) : ring_buffer_(ring_buffer) {}

    void PublishEvent(EventTranslatorInterface<T>* translator) {
        int64_t sequence = ring_buffer_->Next();
        translator->TranslateTo(sequence, ring_buffer_->Get(sequence));
        ring_buffer_->Publish(sequence);
    }

 private:
    RingBuffer<T>* ring_buffer_;
};

};  // namespace disruptor

#endif
