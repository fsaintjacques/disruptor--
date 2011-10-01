// Copyright 2011 <François Saint-Jacques>

#include <climits>
#include <vector>

#include "disruptor/sequence.h"
#include "disruptor/batch_descriptor.h"
#include "disruptor/event.h"

#ifndef DISRUPTOR_INTERFACE_H_ // NOLINT
#define DISRUPTOR_INTERFACE_H_ // NOLINT

namespace disruptor {

class ClaimStrategyInterface {
 public:
    virtual int64_t IncrementAndGet(
            const std::vector<Sequence*>& dependent_sequences) = 0;
    virtual int64_t IncrementAndGet(const int& delta,
            const std::vector<Sequence*>& dependent_sequences) = 0;

    virtual void SetSequence(const int64_t& sequence,
            const std::vector<Sequence*>& dependent_sequences) = 0;

    virtual bool HasAvalaibleCapacity(
        const std::vector<Sequence*>& dependent_sequences) = 0;

    virtual void SerialisePublishing(const int64_t& sequence,
                                     const Sequence& cursor,
                                     const int64_t& batch_size) = 0;
};

class SequenceBarrierInterface {
 public:
    // Wait for the given sequence to be available for consumption.
    //
    // @param sequence to wait for
    // @return the sequence up to which is available
    // @throws AlertException if a status change has occurred for the
    // Disruptor
    // @throws InterruptedException if the thread needs awaking on a
    // condition variable.
    virtual int64_t WaitFor(const int64_t& sequence) = 0;

     // Wait for the given sequence to be available for consumption with a
     // time out.
     //
     // @param sequence to wait for
     // @param timeout in microseconds
     // @return the sequence up to which is available
     // @throws AlertException if a status change has occurred for the
     // Disruptor
     // @throws InterruptedException if the thread needs awaking on a
     // condition variable.
    virtual int64_t WaitFor(const int64_t& sequence,
                          const int64_t& timeout_micro) = 0;

    // Delegate a call to the {@link RingBuffer#getCursor()}
    //  @return value of the cursor for entries that have been published.
    virtual int64_t GetCursor() const = 0;

    // The current alert status for the barrier.
    // @return true if in alert otherwise false.
    virtual bool IsAlerted() const = 0;

    // Alert the {@link EventProcessor}s of a status change and stay in this
    // status until cleared.
    virtual void Alert() = 0;

    // Clear the current alert status.
    virtual void ClearAlert() = 0;
};

template<typename T>
class EventFactoryInterface {
 public:
     virtual T* NewInstance(int size) const = 0;
};

template<typename T>
class EventHandlerInterface {
 public:
    // Called when a publisher has published an event to the {@link RingBuffer}
    // @param event published to the {@link RingBuffer}
    // @param sequence of the event being processed
    // @param end_of_batch flag to indicate if this is the last event in a batch
    // from the {@link RingBuffer}
    // @throws Exception if the EventHandler would like the exception handled
    // further up the chain.
    virtual void OnEvent(T* event, const int64_t& sequence, bool end_of_batch) = 0;
};

template<typename T>
class EventTranslatorInterface {
 public:
     virtual T* TranslateTo(T* event, const int64_t& sequence) = 0;
};

template<typename T>
class EventProcessorInterface {
 public:
     // Get a reference to the {@link Sequence} being used by this
     // {@link EventProcessor}.
     // @return reference to the {@link Sequence} for this
     // {@link EventProcessor}
    virtual Sequence* GetSequence() = 0;

    // Signal that this EventProcessor should stop when it has finished
    // consuming at the next clean break.
    // It will call {@link DependencyBarrier#alert()} to notify the thread to
    // check status.
    virtual void Halt() = 0;
};

class WaitStrategyInterface {
 public:
    //  Wait for the given sequence to be available for consumption in a
    //  {@link RingBuffer}
    //
    //  @param dependents further back the chain that must advance first
    //  @param ringBuffer on which to wait.
    //  @param barrier the consumer is waiting on.
    //  @param sequence to be waited on.
    //  @return the sequence that is available which may be greater than the
    //  requested sequence.
    //
    //  @throws AlertException if the status of the Disruptor has changed.
    //  @throws InterruptedException if the thread is interrupted.
    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t& sequence) = 0;

    //  Wait for the given sequence to be available for consumption in a
    //  {@link RingBuffer} with a timeout specified.
    //
    //  @param dependents further back the chain that must advance first
    //  @param ringBuffer on which to wait.
    //  @param barrier the consumer is waiting on.
    //  @param sequence to be waited on.
    //  @param timeout value to abort after.
    //  @param units of the timeout value.
    //  @return the sequence that is available which may be greater than the
    //  requested sequence.
    //
    //  @throws AlertException if the status of the Disruptor has changed.
    //  @throws InterruptedException if the thread is interrupted.
    virtual int64_t WaitFor(const std::vector<Sequence*>& dependents,
                            const Sequence& cursor,
                            const SequenceBarrierInterface& barrier,
                            const int64_t & sequence,
                            const int64_t & timeout_micros) = 0;

    // Signal those waiting that the {@link RingBuffer} cursor has advanced.
    virtual void SignalAllWhenBlocking() = 0;
};

template<typename T>
int64_t GetMinimumSequence(
        const std::vector<EventProcessorInterface<T>*>& event_processors) {
        int64_t minimum = LONG_MAX;

        for (EventProcessorInterface<T>* event_processor: event_processors) {
            int64_t sequence = event_processor->GetSequence()->sequence();
            minimum = minimum < sequence ? minimum : sequence;
        }

        return minimum;
};

int64_t GetMinimumSequence(
        const std::vector<Sequence*>& sequences) {
        int64_t minimum = LONG_MAX;

        for (Sequence* sequence_: sequences) {
            int64_t sequence = sequence_->sequence();
            minimum = minimum < sequence ? minimum : sequence;
        }

        return minimum;
};

};  // namespace disruptor

#endif // DISRUPTOR_INTERFACE_H_ NOLINT
