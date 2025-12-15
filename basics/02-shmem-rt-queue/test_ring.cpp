#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "ring.h"

struct Task {
  std::string payload;
  std::uint64_t exec_time_ns{0}; // filled by consumer
};

int main(int argc, char** argv) {
  std::size_t capacity = 8;
  int count = 10;
  std::string prod_log_path = "producer_rt.log";
  std::string cons_log_path = "consumer_rt.log";
  if (argc >= 2) capacity = static_cast<std::size_t>(std::stoul(argv[1]));
  if (argc >= 3) count = std::stoi(argv[2]);
  if (argc >= 4) prod_log_path = argv[3];
  if (argc >= 5) cons_log_path = argv[4];

  SpscRingBuffer ring(capacity);
  std::ofstream prod_log(prod_log_path, std::ios::out | std::ios::trunc);
  std::ofstream cons_log(cons_log_path, std::ios::out | std::ios::trunc);

  auto drain_pop = [&]() {
    void* ptr = nullptr;
    while (ring.Pop(ptr)) {
      auto* t = static_cast<Task*>(ptr);
      if (prod_log) {
        prod_log << "[producer] recv " << t->payload
                 << " exec_ns=" << t->exec_time_ns << "\n";
        prod_log.flush();
      }
      delete t;
    }
  };

  std::thread producer([&] {
    for (int i = 0; i < count; ++i) {
      drain_pop();
      auto* task = new Task{.payload = "msg_" + std::to_string(i)};
      while (true) {
        if (ring.Push(static_cast<void*>(task))) {
          break;
        }
        // queue full, try to pop completed items to make space
        drain_pop();
      }
      if (prod_log) {
        prod_log << "[producer] push " << task->payload << "\n";
        prod_log.flush();
      }
    }
    // Pop until tail catches head; consumer may still be finishing last item.
    while (ring.TailCount() < ring.HeadCount()) {
      drain_pop();
    }
    ring.Stop();
  });

  std::thread consumer([&] {
    while (true) {
      void* ptr = nullptr;
      std::size_t idx = 0;
      if (ring.Get(ptr, idx)) {
        auto* t = static_cast<Task*>(ptr);
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto end = std::chrono::steady_clock::now();
        t->exec_time_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count();
        ring.Finish(idx, t);
        if (cons_log) {
          cons_log << "[consumer] got " << t->payload << "\n";
          cons_log.flush();
        }
        continue;
      }
      if (ring.IsStopped()) {
        break;
      }
    }
  });

  producer.join();
  consumer.join();
  return 0;
}


