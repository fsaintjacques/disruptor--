#include <sys/time.h>

#include <exception>
#include <functional>
#include <iostream>
#include <thread>

#include <disruptor/ring_buffer.h>
#include <disruptor/event_processor.h>

struct DummyEvent {
    DummyEvent() : count(0) {};
    int count;
};

using namespace disruptor;

class DummyBatchHandler : public EventHandlerInterface<DummyEvent> {
 public:
    virtual void OnEvent(DummyEvent* event, bool end_of_batch) {
        if (event)
            event->count += 1;
    };
};

class DummyEventFactory : public EventFactoryInterface<DummyEvent> {
 public:
    virtual DummyEvent* Create() const {
        return new DummyEvent();
    };
};



int main(int arc, char** argv) {
    int buffer_size = 1024 * 8;
    long iterations = 1000L * 1000L * 100L;

    DummyEventFactory dummy_factory;
    RingBuffer<DummyEvent> ring_buffer(kSingleThreadedStrategy,
                                     kYieldingStrategy,
                                     buffer_size,
                                     dummy_factory);

    std::vector<EventProcessorInterface<DummyEvent>*> processor_list(0);
    ProcessingSequenceBarrier* barrier = \
        ring_buffer.SetTrackedProcessor(processor_list);

    DummyBatchHandler dummy_handler;
    BatchEventProcessor<DummyEvent> processor(&ring_buffer,
                                            (SequenceBarrierInterface*) barrier,
                                            &dummy_handler);

    std::thread consumer(std::ref<BatchEventProcessor<DummyEvent>>(processor));

    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    for (long i=0; i<iterations; i++) {
        Event<DummyEvent>* event = ring_buffer.NextEvent();
        DummyEvent* toutoune = event->data();
        toutoune->count = i;
        event->set_data(toutoune);
        ring_buffer.Publish(event->sequence());
    }

    long expected_sequence = ring_buffer.GetCursor();
    while (processor.GetSequence()->sequence() < expected_sequence) {}

    gettimeofday(&end_time, NULL);

    double start, end;
    start = start_time.tv_sec + ((double) start_time.tv_usec / 1000000);
    end = end_time.tv_sec + ((double) end_time.tv_usec / 1000000);

    std::cout.precision(15);
    std::cout << (iterations * 1.0) / (end - start)
              << " operations / seconds" << std::endl;

    barrier->Alert();
    consumer.join();

    delete barrier;

    return 0;
}

