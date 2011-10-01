#include <sys/time.h>

#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <disruptor/ring_buffer.h>
#include <disruptor/event_publisher.h>
#include <disruptor/event_processor.h>

struct DummyEvent {
    DummyEvent() : count(0) {};
    int count;
};

using namespace disruptor;

class DummyBatchHandler : public EventHandlerInterface<DummyEvent> {
 public:
    virtual void OnEvent(DummyEvent* event, const int64_t& sequence, bool end_of_batch) {
        if (event)
            event->count += 1;
    };
};

class DummyEventFactory : public EventFactoryInterface<DummyEvent> {
 public:
    virtual DummyEvent NewInstance() const {
        return DummyEvent();
    };
};

class DummyEventTranslator : public EventTranslatorInterface<DummyEvent> {
 public:
    virtual DummyEvent* TranslateTo(DummyEvent* event, const int64_t& sequence) {
        event->count=sequence;
        return event;
    };
};

int main(int arc, char** argv) {
    int buffer_size = 1024 * 8;
    long iterations = 1000L * 1000L * 300L;

    DummyEventFactory dummy_factory;
    RingBuffer<DummyEvent> ring_buffer(&dummy_factory,
                                       buffer_size,
                                       kSingleThreadedStrategy,
                                       kYieldingStrategy);

    std::vector<Sequence*> sequence_to_track(0);
    std::unique_ptr<ProcessingSequenceBarrier> barrier(
        ring_buffer.SetTrackedProcessor(sequence_to_track));

    DummyBatchHandler dummy_handler;
    BatchEventProcessor<DummyEvent> processor(&ring_buffer,
                                              (SequenceBarrierInterface*) barrier.get(),
                                              &dummy_handler);

    std::thread consumer(std::ref<BatchEventProcessor<DummyEvent>>(processor));

    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    std::unique_ptr<DummyEventTranslator> translator(new DummyEventTranslator);
    EventPublisher<DummyEvent> publisher(&ring_buffer);
    for (long i=0; i<iterations; i++) {
        publisher.PublishEvent(translator.get());
    }

    long expected_sequence = ring_buffer.GetCursor();
    while (processor.GetSequence()->sequence() < expected_sequence) {}

    gettimeofday(&end_time, NULL);

    double start, end;
    start = start_time.tv_sec + ((double) start_time.tv_usec / 1000000);
    end = end_time.tv_sec + ((double) end_time.tv_usec / 1000000);

    std::cout.precision(15);
    std::cout << "1P-1EP performance: ";
    std::cout << (iterations * 1.0) / (end - start)
              << " ops/secs" << std::endl;

    barrier->Alert();
    consumer.join();

    return EXIT_SUCCESS;
}

