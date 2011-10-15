// Copyright 2011 <François Saint-Jacques>

#ifndef DISRUPTOR_WORKPROCESSOR_H_  // NOLINT
#define DISRUPTOR_WORKPROCESSOR_H_  // NOLINT

namespace disruptor {

template<typename T>
class WorkProcessor : public EventProcessorInterface<T> {
 public:
    WorkProcessor(RingBuffer<T>* ring_buffer,
                  SequenceBarrierInterface* sequence_barrier,
                  WorkHandlerInterface<T>* work_handler
                  const long& work_sequence) :
            running_(false),
            ring_buffer_(ring_buffer),
            sequence_barrier_(sequence_barrier),
            work_handler_(work_handler),
            work_sequence_(work_sequence) {}

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
        work_handler_->OnStart();

        bool processed_sequence = true;
        T* event = NULL;
        int64_t next_sequence = sequence_.sequence();

        while (true) {
            try {
                if (processed_sequence) {
                    processed_sequence = false;
                    next_sequence = work_sequence_.IncrementAndGet(1L);
                    sequence_.set_sequence(next_sequence - 1L);
                }

                sequence_barrier_->WaitFor(next_sequence);
                event = ring_buffer_->Get(next_sequence);
                work_handler_->OnEvent(event);

                processed_sequence = true;
            } catch(const AlertException& e) {
                if (running_.load())
                    break;
            } catch(const std::exception& e) {
                // TODO(fsaintjacques): exception_handler_->handle(e, event);
                processed_sequence = true;
            }
        }

        work_handler_->OnShutDown();
        running_.store(false);
    }


 private:
    std::atomic<bool> running_;
    Sequence sequence_;
    PaddedLong work_sequence_;

    RingBuffer<T>* ring_buffer_;
    SequenceBarrierInterface* sequence_barrier_;
    WorkHandlerInterface<T>* work_handler_;
};

}; // namespace disruptor

#endif // DISRUPTOR_WORKHANDLER_H_
