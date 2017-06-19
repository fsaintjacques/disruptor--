#include <climits>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <typeinfo>
#include <vector>

#include "args.hxx"

#include "disruptor/sequencer.h"

#ifdef __GNUG__  // gnu C++ compiler

#include <cxxabi.h>

std::string demangle(const char* mangled_name) {
  std::size_t len = 0;
  int status = 0;
  std::unique_ptr<char, decltype(&std::free)> ptr(
      __cxxabiv1::__cxa_demangle(mangled_name, nullptr, &len, &status),
      &std::free);
  return ptr.get();
}

#else

std::string demangle(const char* name) { return name; }

#endif  // _GNUG_

size_t RING_BUFFER_SIZE = 2048;

// Not handled in a thread safe way
unsigned long sum = 0;
// Number of times to loop over buffer in a producer thread
size_t counter = 100000;
// Batch size
size_t delta = 1;

// Number of producer threads
int np = 1;
// Number of consumer threads
int nc = 1;

// Not handled in a thread safe way
bool running = true;

// Consume published data
template <typename T, typename C, typename W>
void consume(disruptor::Sequencer<T, C, W>& s, disruptor::Sequence& seq) {
  std::vector<disruptor::Sequence*> depseqs;
  auto barrier = s.NewBarrier(depseqs);

  int64_t next_seq = disruptor::kFirstSequenceValue;

  int exitCtr = 0;

  while (running) {
#ifdef PRINT_DEBUG_CONS
    std::stringstream ss;
    ss << "Wait for next seq: " << next_seq << ' ' << std::this_thread::get_id()
       << '\n';
    std::cout << ss.str();
#endif

    int64_t available_seq =
        barrier->WaitFor(next_seq, std::chrono::microseconds(10000));

#ifdef PRINT_DEBUG_CONS
    ss.clear();
    ss.str(std::string());
    ss << "Available seq: " << available_seq << ' '
       << std::this_thread::get_id() << '\n';
    std::cout << ss.str();
#endif

    if (available_seq == disruptor::kTimeoutSignal && running == false) break;

    if (available_seq < next_seq) continue;

    // Only required for claim strategy MultiThreadedStrategyEx as it changes
    // the cursor.
    available_seq = s.GetHighestPublishedSequence(next_seq, available_seq);
    if (available_seq < next_seq) {
      ++exitCtr;

#ifdef PRINT_DEBUG_CONS
      ss.clear();
      ss.str(std::string());
      ss << "Highest published seq: " << available_seq << ' '
         << std::this_thread::get_id() << '\n';
      std::cout << ss.str();

      if (exitCtr > 50)
        break;
#endif
      // Otherwise goes into a busy loop with blocking strategy
      if (exitCtr > 10)
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      continue;
    }

    for (int64_t i = next_seq; i <= available_seq; ++i) {
      const long& ev = s[i];
#ifdef PRINT_DEBUG_CONS
      std::cout << i << " Event: " << ev << '\n';
#endif
      sum += ev;
    }

    seq.set_sequence(available_seq);

    next_seq = available_seq + 1;

    exitCtr = 0;
  }

  barrier->set_alerted(true);
}

// Publish data
template <typename T, typename C, typename W>
void produce(disruptor::Sequencer<T, C, W>& s) {
  int iterations = counter * RING_BUFFER_SIZE;

  for (int64_t i = 0; i < iterations; ++i) {
    if (running == false) break;

    int64_t sequence = s.Claim(delta);

    if (sequence < delta) {
      for (int k = 0; k <= sequence; ++k) {
        s[k] = k;
#ifdef PRINT_DEBUG_PROD
        std::stringstream ss;
        ss << "Publish seq: " << (k) << ' ' << std::this_thread::get_id()
           << '\n';
        std::cout << ss.str();
#endif
      }

      s.Publish(sequence, delta);
      continue;
    }

    for (int64_t k = sequence; k > sequence - delta; --k) {
      s[k] = k;

#ifdef PRINT_DEBUG_PROD
      std::stringstream ss;
      ss << "Publish seq: " << (k) << ' ' << std::this_thread::get_id() << '\n';
      std::cout << ss.str();
#endif
    }

    s.Publish(sequence, delta);
  }
}

template <typename T, typename C, typename W>
void runOnce() {
  std::cout << "Staring run " << std::endl;

  running = true;

  disruptor::Sequence sequences[nc];

  std::vector<disruptor::Sequence*> seqs;
  disruptor::Sequencer<T, C, W> s(RING_BUFFER_SIZE);

  for (int i = 0; i < nc; ++i) seqs.push_back(&sequences[i]);

  s.set_gating_sequences(seqs);

  std::thread* tc = new std::thread[nc];
  for (int i = 0; i < nc; ++i)
    tc[i] = std::thread(consume<T, C, W>, std::ref(s), std::ref(sequences[i]));

  std::thread* tp = new std::thread[np];

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < np; ++i)
    tp[i] = std::thread(produce<T, C, W>, std::ref(s));

  // std::this_thread::sleep_for(std::chrono::seconds(3));

  for (int i = 0; i < np; ++i) tp[i].join();
  running = false;
  for (int i = 0; i < nc; ++i) tc[i].join();

  int64_t cursor = s.GetCursor();
  unsigned long snapSum = sum;
  std::cout << "\nBatch size: " << delta
            << " Ring buffer size: " << RING_BUFFER_SIZE << '\n';
  std::cout << "Cursor: " << cursor << '\n';
  std::cout << "Sum: " << snapSum << '\n';

  gettimeofday(&end_time, NULL);

  double start, end;
  start = start_time.tv_sec + ((double)start_time.tv_usec / 1000000);
  end = end_time.tv_sec + ((double)end_time.tv_usec / 1000000);

  std::cout.precision(15);
  std::cout << np << "P-" << nc << "C " << demangle(typeid(C).name()) << " "
            << demangle(typeid(W).name()) << '\n';
  std::cout << (cursor * 1.0) / (end - start) << " ops/secs"
            << "\n\n";
}

int main(int argc, char** argv) {
  std::cout.imbue(std::locale(""));

  args::ArgumentParser parser(
      "This is an example program that demonstrates disruptor-- usage.", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<int> num_prod(parser, "num_prod", "Number of producers",
                                {"np"}, np);
  args::ValueFlag<int> num_cons(parser, "num_cons", "Number of consumers",
                                {"nc"}, nc);
  args::ValueFlag<size_t> batch_size(parser, "batch_size", "Batch size",
                                     {"delta"}, delta);
  args::ValueFlag<int> multi(parser, "multi",
                             "Multithreaded claim strategy (1 old, 2 new)",
                             {"mt"}, false);
  args::ValueFlag<size_t> looper(parser, "looper",
                                 "Number of times loop over the ring buffer.",
                                 {'l', "loop"}, counter);
  args::ValueFlag<size_t> ring_buffer_size(
      parser, "ring_buffer_size", "Ring buffer size", {"rb"}, RING_BUFFER_SIZE);

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  delta = batch_size.Get();
  counter = looper.Get();
  nc = num_cons.Get();
  RING_BUFFER_SIZE = ring_buffer_size.Get();

  if (multi.Get() == 0) {
    runOnce<long, disruptor::SingleThreadedStrategy,
            disruptor::SleepingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, disruptor::SingleThreadedStrategy,
            disruptor::YieldingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, disruptor::SingleThreadedStrategy,
            disruptor::BusySpinStrategy>();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, disruptor::SingleThreadedStrategy,
            disruptor::BlockingStrategy>();

  } else {
    np = num_prod.Get();

    switch (multi.Get()) {
      case 1: {
        runOnce<long, disruptor::MultiThreadedStrategy,
                disruptor::SleepingStrategy<> >();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategy,
                disruptor::YieldingStrategy<> >();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategy,
                disruptor::BusySpinStrategy>();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategy,
                disruptor::BlockingStrategy>();
      } break;

      case 2: {
        runOnce<long, disruptor::MultiThreadedStrategyEx,
                disruptor::SleepingStrategy<> >();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategyEx,
                disruptor::YieldingStrategy<> >();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategyEx,
                disruptor::BusySpinStrategy>();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        runOnce<long, disruptor::MultiThreadedStrategyEx,
                disruptor::BlockingStrategy>();
      } break;
    }
  }

  return 0;
}
