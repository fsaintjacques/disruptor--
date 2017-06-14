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

const size_t RING_BUFFER_SIZE = 2048;

// Do not care about thread safety here.
unsigned long sum = 0;
// Number of times to loop over buffer in a producer thread
size_t counter = 100000;
// Batch size
size_t delta = 1;

std::array<long, RING_BUFFER_SIZE> events;

int np = 1;
int nc = 1;

// Ignoring thread safety
bool running = true;

// Consume published data
template <typename T, size_t N, typename C, typename W>
void consume(disruptor::Sequencer<T, N, C, W>& s, disruptor::Sequence& seq) {
  std::vector<disruptor::Sequence*> depseqs;
  auto barrier = s.NewBarrier(depseqs);

  int64_t next_seq = disruptor::kFirstSequenceValue;

  while (running) {
    /*std::stringstream ss;
    ss << "Wait for next seq: " << next_seq << ' ' << std::this_thread::get_id()
       << '\n';
    std::cout << ss.str();*/

    int64_t available_seq = barrier->WaitFor(next_seq);

    /*ss.clear();
    ss.str() = "";
    ss << "Available seq: " << available_seq << ' '
       << std::this_thread::get_id() << '\n';
    std::cout << ss.str();*/

    if (available_seq <= next_seq) continue;

    for (int i = next_seq; i <= available_seq; ++i) {
      const long& ev = s[i];
      // std::cout << i << " Event: " <<ev << '\n';
      sum += ev;
    }

    seq.set_sequence(available_seq);

    next_seq = available_seq + 1;
  }

  barrier->set_alerted(true);
}

// Publish data
template <typename T, size_t N, typename C, typename W>
void produce(disruptor::Sequencer<T, N, C, W>& s) {
  int iterations = counter * RING_BUFFER_SIZE;

  for (int i = 0; i < iterations; ++i) {
    if (running == false) break;

    int64_t sequence = s.Claim(delta);

    if (sequence < delta) {
      for (int k = 0; k <= sequence; ++k) {
        s[k] = k;

        /*std::stringstream ss;
        ss << "Publish seq: " << (k) << ' ' << std::this_thread::get_id()
           << '\n';
        std::cout << ss.str();*/
      }

      s.Publish(sequence, delta);
      continue;
    }

    for (int k = sequence; k > sequence - delta; --k) {
      s[k] = k;

      /*std::stringstream ss;
      ss << "Publish seq: " << (k) << ' ' << std::this_thread::get_id() << '\n';
      std::cout << ss.str();*/
    }

    s.Publish(sequence, delta);
  }
}

template <typename T, size_t N, typename C, typename W>
void runOnce() {
  running = true;

  disruptor::Sequence sequences[nc];

  std::vector<disruptor::Sequence*> seqs;
  disruptor::Sequencer<T, N, C, W> s(events);

  for (int i = 0; i < nc; ++i) seqs.push_back(&sequences[i]);

  s.set_gating_sequences(seqs);

  std::thread* tc = new std::thread[nc];
  for (int i = 0; i < nc; ++i)
    tc[i] =
        std::thread(consume<T, N, C, W>, std::ref(s), std::ref(sequences[i]));

  std::thread* tp = new std::thread[np];
  for (int i = 0; i < np; ++i)
    tp[i] = std::thread(produce<T, N, C, W>, std::ref(s));

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  running = false;

  int64_t cursor = s.GetCursor();
  unsigned long snapSum = sum;
  std::cout << "\nBatch size: " << delta << '\n';
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

  for (int i = 0; i < np; ++i) tp[i].join();

  for (int i = 0; i < nc; ++i) tc[i].join();
}

int main(int argc, char* argv[]) {
  std::cout.imbue(std::locale(""));

  args::ArgumentParser parser(
      "This is an example program that demonstrates disruptor-- usage.", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<int> num_prod(parser, "1", "Number of producers", {"nc"});
  args::ValueFlag<int> num_cons(parser, "1", "Number of consumers", {"np"});
  args::ValueFlag<size_t> batch_size(parser, "1", "Batch size", {"delta"});
  args::ValueFlag<bool> multi(parser, "false", "Multithreaded claim strategy",
                              {"mt"});

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

  if (multi.Get()) {
    nc = num_prod.Get();
    np = num_cons.Get();

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::SleepingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::YieldingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::BusySpinStrategy>();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::BlockingStrategy>();

  } else {
    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::SleepingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::YieldingStrategy<> >();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::BusySpinStrategy>();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    runOnce<long, RING_BUFFER_SIZE,
            disruptor::MultiThreadedStrategy<RING_BUFFER_SIZE>,
            disruptor::BlockingStrategy>();
  }

  return 0;
}
