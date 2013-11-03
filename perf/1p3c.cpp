#include <iostream>
#include <vector>
#include <memory>
#include <thread>

#include <disruptor/ring_buffer.h>
#include <disruptor/exception_handler.h>
#include <disruptor/event_processor.h>
#include <disruptor/event_publisher.h>

#include "../test/support/stub_event.h"

using namespace disruptor;
using namespace std;

int main(int argc, char* argv[])
{
  test::StubEventFactory factory;
  RingBuffer<test::StubEvent> rb(&factory, 1024, kSingleThreadedStrategy, kBusySpinStrategy);

  vector<Sequence*> dependencies;
  std::unique_ptr<ProcessingSequenceBarrier> barrier(rb.NewBarrier(dependencies));

  test::StubBatchHandler handler;
  IgnoreExceptionHandler<test::StubEvent> exceptionHandler;

  BatchEventProcessor<test::StubEvent> processor1(&rb, barrier.get(), &handler, &exceptionHandler);
  BatchEventProcessor<test::StubEvent> processor2(&rb, barrier.get(), &handler, &exceptionHandler);
  BatchEventProcessor<test::StubEvent> processor3(&rb, barrier.get(), &handler, &exceptionHandler);

  std::thread consumer1(std::ref<BatchEventProcessor<test::StubEvent>>(processor1));
  std::thread consumer2(std::ref<BatchEventProcessor<test::StubEvent>>(processor2));
  std::thread consumer3(std::ref<BatchEventProcessor<test::StubEvent>>(processor3));

  std::unique_ptr<test::StubEventTranslator> translator(new test::StubEventTranslator);
  EventPublisher<test::StubEvent> publisher(&rb);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  const int iterations = 1000;
  for (int i = 0; i < iterations; ++i)
    publisher.PublishEvent(translator.get());

  long expected_sequence = rb.GetCursor();
  while (processor1.GetSequence()->sequence() < expected_sequence) {}
  while (processor2.GetSequence()->sequence() < expected_sequence) {}
  while (processor3.GetSequence()->sequence() < expected_sequence) {}

  struct timeval end_time;
  gettimeofday(&end_time, NULL);

  double start, end;
  start = start_time.tv_sec + ((double) start_time.tv_usec / 1000000);
  end = end_time.tv_sec + ((double) end_time.tv_usec / 1000000);

  std::cout.precision(15);
  std::cout << "1P3C unicast performance: ";
  std::cout << (iterations * 1.0) / (end - start)
              << " ops/secs" << std::endl;

  processor1.Halt();
  processor2.Halt();
  processor3.Halt();
  consumer1.join();
  consumer2.join();
  consumer3.join();

  return 0;
}

