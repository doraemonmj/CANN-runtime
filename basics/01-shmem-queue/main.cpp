#include <fstream>
#include <iostream>
#include <string>
#include <thread>

struct Task {
  std::string payload;
};

// Minimal atomic wrapper using fetch_add semantics (no CAS), enough for SPSC.
struct AtomicSize {
  volatile std::size_t v{0};

  std::size_t LoadAcquire() const {
    std::size_t val = __sync_fetch_and_add(const_cast<volatile std::size_t*>(&v), 0);
    return val;
  }

  void StoreRelease(std::size_t x) {
    __sync_lock_test_and_set(&v, x); // atomic exchange
  }
};

struct AtomicBool {
  volatile int v{0};

  bool LoadAcquire() const {
    int val = __sync_fetch_and_add(const_cast<volatile int*>(&v), 0);
    return val != 0;
  }

  void StoreRelease(bool x) {
    __sync_lock_test_and_set(&v, x ? 1 : 0);
  }
};

// Simple bounded blocking queue storing Task* in a fixed-size buffer.
class BlockingQueue {
public:
  explicit BlockingQueue(std::size_t capacity)
      : capacity_(capacity), buffer_(new Task *[capacity]) {}

  ~BlockingQueue() {
    // Drain remaining tasks to avoid leaks.
    std::size_t h = head_.LoadAcquire();
    const std::size_t t = tail_.LoadAcquire();
    while (h != t) {
      delete buffer_[h];
      h = (h + 1) % capacity_;
    }
    delete[] buffer_;
  }

  // Producer tries to push; returns true on success, false if full or stopped.
  bool Push(Task *task) {
    if (stop_.LoadAcquire()) {
      return false;
    }
    const std::size_t head = head_.LoadAcquire();
    const std::size_t tail = tail_.LoadAcquire();
    const std::size_t next = (tail + 1) % capacity_;
    if (next == head) {
      return false; // full
    }
    buffer_[tail] = task;
    tail_.StoreRelease(next);
    return true;
  }

  // Consumer tries to pop; returns true and sets out on success, false if empty.
  bool Pop(Task*& out) {
    const std::size_t tail = tail_.LoadAcquire();
    const std::size_t head = head_.LoadAcquire();
    if (head == tail) {
      return false;
    }
    out = buffer_[head];
    head_.StoreRelease((head + 1) % capacity_);
    return true;
  }

  bool IsStopped() {
    return stop_.LoadAcquire();
  }

  void Stop() {
    stop_.StoreRelease(true);
  }

private:
  const std::size_t capacity_;
  Task** buffer_;
  AtomicSize head_{};
  AtomicSize tail_{};
  AtomicBool stop_{};
};

int main(int argc, char **argv) {
  std::size_t capacity = 8; // default queue length
  int count = 10;           // default number of messages
  std::string prod_log_path = "producer.log";
  std::string cons_log_path = "consumer.log";
  if (argc >= 2) {
    capacity = static_cast<std::size_t>(std::stoul(argv[1]));
  }
  if (argc >= 3) {
    count = std::stoi(argv[2]);
  }
  if (argc >= 4) {
    prod_log_path = argv[3];
  }
  if (argc >= 5) {
    cons_log_path = argv[4];
  }

  BlockingQueue queue(capacity);
  std::ofstream prod_log(prod_log_path, std::ios::out | std::ios::trunc);
  std::ofstream cons_log(cons_log_path, std::ios::out | std::ios::trunc);

  std::thread producer([&] {
    for (int i = 0; i < count; ++i) {
      auto* task = new Task{.payload = "msg_" + std::to_string(i)};
      while (!queue.Push(task)) {
        // busy wait retry
      }
      if (prod_log) {
        prod_log << "[producer] push " << task->payload << "\n";
        prod_log.flush();
      }
    }
    queue.Stop();
  });

  std::thread consumer([&] {
    while (true) {
      Task* t = nullptr;
      if (queue.Pop(t)) {
        if (cons_log) {
          cons_log << "[consumer] pop " << t->payload << "\n";
          cons_log.flush();
        }
        delete t;
        continue;
      }
      if (queue.IsStopped()) {
        break;
      }
    }
  });

  producer.join();
  consumer.join();
  return 0;
}

