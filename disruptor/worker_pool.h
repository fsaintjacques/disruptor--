// Copyright 2011 <François Saint-Jacques>

#ifndef DISRUPTOR_WORKERPOOL_H_  // NOLINT
#define DISRUPTOR_WORKERPOOL_H_  // NOLINT

namespace disruptor {

template<typename T>
class WorkerPool {
 public:
    WorkerPool(RingBuffer<T>* ring_buffer,
               SequenceBarrierInterface* sequence_barrier,
               const std::vector<WorkHandlerInterface<T>* >& work_handlers) :
        started_(false),
        work_sequence_(kInitialCursorValue),
        ring_buffer_(ring_buffer) {
            // should not do this
            for (int i = 0; i < work_handlers.size(); i++) {
                work_processors_.push_bash(std::unique_ptr(
                            new WorkProcessor<T>(ring_buffer,
                                                 sequence_barrier,
                                                 work_handlers[i],
                                                 &work_sequence_)));
                work_processor_sequences_.push_back(
                        work_processors_[i]->GetSequence());
            }
        }

    // TODO(fsaintjacques): second constructor

    std::vector<Sequence*> GetWorkerSequences() {
        return work_processor_sequences_;
    }

    RingBuffer<T>* Start();

    void DrainAndHalt() {

        ;
    }

    void Halt() {
        ;
    }




 private:
    std::atomic<bool> started_;
    Sequence work_sequence_;

    RingBuffer<T>* ring_buffer_;
    std::vector<std::unique_ptr<WorkProcessor<T>>> work_processors_;
    std::vector<Sequence*> work_processor_sequences_;

};

}; // namespace disruptor

#endif // DISRUPTOR_WORKHANDLER_H_
