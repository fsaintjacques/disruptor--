#include <sys/time.h>

#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <disruptor/ring_buffer.h>
#include <disruptor/event_publisher.h>
#include <disruptor/event_processor.h>
#include <disruptor/exception_handler.h>

#include "../test/support/stub_event.h"

using namespace disruptor;

int main(int arc, char** argv) {
    int buffer_size = 1024 * 8;
    long iterations = 1000L * 1000L * 300;

    test::StubEventFactory stub_factory;
    RingBuffer<test::StubEvent> ring_buffer(&stub_factory,
                                       buffer_size,
                                       kSingleThreadedStrategy,
                                       kBusySpinStrategy);

    std::vector<Sequence*> sequence_to_track(0);
    std::unique_ptr<ProcessingSequenceBarrier> barrier(
        ring_buffer.NewBarrier(sequence_to_track));

    test::StubBatchHandler stub_handler;
    IgnoreExceptionHandler<test::StubEvent> stub_exception_handler;
    BatchEventProcessor<test::StubEvent> processor(&ring_buffer,
                                              (SequenceBarrierInterface*) barrier.get(),
                                              &stub_handler,
                                              &stub_exception_handler);

    std::thread consumer(std::ref<BatchEventProcessor<test::StubEvent>>(processor));

    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    std::unique_ptr<test::StubEventTranslator> translator(new test::StubEventTranslator);
    EventPublisher<test::StubEvent> publisher(&ring_buffer);
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
    std::cout << "1P-1EP-UNICAST performance: ";
    std::cout << (iterations * 1.0) / (end - start)
              << " ops/secs" << std::endl;

    processor.Halt();
    consumer.join();

    return EXIT_SUCCESS;
}

