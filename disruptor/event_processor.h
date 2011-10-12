// Copyright 2011 <François Saint-Jacques>
#include <exception>

#include "disruptor/ring_buffer.h"
#ifndef DISRUPTOR_EVENT_PROCESSOR_H_ // NOLINT
#define DISRUPTOR_EVENT_PROCESSOR_H_ // NOLINT

namespace disruptor {

template <typename T>
class BatchEventProcessor : public EventProcessorInterface<T> {
 public:
    BatchEventProcessor(RingBuffer<T>* ring_buffer,
                        SequenceBarrierInterface* sequence_barrier,
                        EventHandlerInterface<T>* event_handler) :
            running_(false),
            ring_buffer_(ring_buffer),
            sequence_barrier_(sequence_barrier),
            event_handler_(event_handler) {}


    virtual Sequence* GetSequence() { return &sequence_; }

    virtual void Halt() {
        running_.store(false);
        sequence_barrier_->Alert();
    }

    virtual void Run() {
        if (running_.load())
            throw std::runtime_error("Thread is already running");

        running_.store(true);
        sequence_barrier_->ClearAlert();
        event_handler_->OnStart();

        T* event = NULL;
        int64_t next_sequence = sequence_.sequence() + 1L;

        while (true) {
            try {
                int64_t avalaible_sequence = \
                    sequence_barrier_->WaitFor(next_sequence);

                while (next_sequence <= avalaible_sequence) {
                    event = ring_buffer_->Get(next_sequence);
                    event_handler_->OnEvent(event, next_sequence,
                            next_sequence == avalaible_sequence);
                    next_sequence++;
                }

                sequence_.set_sequence(next_sequence - 1L);
            } catch(const AlertException& e) {
                if (running_.load())
                    break;
            } catch(const std::exception& e) {
                //TODO(fsaintjacques): exception_handler_->handle(e, event);
                sequence_.set_sequence(next_sequence);
                next_sequence++;
            }
        }

        event_handler_->OnShutdown();
        running_.store(false);
    }

    void operator()() { Run(); }

 private:
    std::atomic<bool> running_;
    Sequence sequence_;

    RingBuffer<T>* ring_buffer_;
    SequenceBarrierInterface* sequence_barrier_;
    EventHandlerInterface<T>* event_handler_;

    DISALLOW_COPY_AND_ASSIGN(BatchEventProcessor);
};


};  // namespace disruptor

#endif // DISRUPTOR_EVENT_PROCESSOR_H_ NOLINT
