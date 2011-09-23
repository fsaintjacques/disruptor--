#include <iostream>

#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <disruptor/ring_buffer.h>
#include <disruptor/event_processor.h>

struct Toutoune {
    int count;
};

using namespace disruptor;

class DummyBatchHandler : public EventHandlerInterface<Toutoune> {
 public:
    virtual void OnEvent(Toutoune* event, bool end_of_batch) {
        if (event)
            std::cerr << "processing: " << event->count << std::endl;
    };
};



int main(int arc, char** argv) {
    int buffer_size = 1024 * 8;
    long iterations = 1000L * 1000L * 300L;

    RingBuffer<Toutoune> ring_buffer(kSingleThreadedStrategy,
                                     kYieldingStrategy,
                                     buffer_size);

    std::vector<EventProcessorInterface<Toutoune>*> processor_list(0);
    ProcessingSequenceBarrier* barrier = \
        ring_buffer.SetTrackedProcessor(processor_list);

    DummyBatchHandler dummy_handler;
    BatchEventProcessor<Toutoune>* processor = \
        new BatchEventProcessor<Toutoune>(&ring_buffer,
                                          (SequenceBarrierInterface*) barrier,
                                          &dummy_handler);

    boost::thread consumer(boost::ref< BatchEventProcessor<Toutoune> >(*processor));

    struct timeval start_time, end_time;

    gettimeofday(&end_time, NULL);

    for (long i=0; i<iterations; i++) {
        Event<Toutoune>* event = ring_buffer.NextEvent();
        Toutoune* toutoune = new Toutoune();
        toutoune->count = i;
        event->set_data(toutoune);
        ring_buffer.Publish(event->sequence());
    }

    long expected_sequence = ring_buffer.GetCursor();
    while (processor->GetSequence()->sequence() < expected_sequence) {}

    gettimeofday(&end_time, NULL);

    std::cout << (iterations * 1.0) / (end_time.tv_sec - start_time.tv_sec)
              << " operations / seconds" << std::endl;

    return 0;
}

