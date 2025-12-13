#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <cstddef>

struct Runable {
    int data_length;     // size of data buffer in bytes
    void* data;          // caller-managed buffer, worker reads/writes in-place
    int binary_length;   // size of binary path buffer
    void* binary;        // pointer to null-terminated shared library path
    int entry_length;    // size of entry point buffer
    const char* entry_point; // pointer to null-terminated symbol name
};

struct Task {
    int id;
    Runable runable;
};

class Machine {
public:
  Machine();
  ~Machine();

  void *Malloc(size_t size);
  void Free(void *ptr);

  void PushTask(const Task &task); // push task to dispatch queue
  bool PopTask(Task &task); // pop task from finish queue, returns false on stop

private:
  void WorkerLoop();

  std::queue<Task> dispatch_queue_;
  std::queue<Task> finish_queue_;
  std::mutex mutex_;
  std::condition_variable dispatch_cv_;
  std::condition_variable finish_cv_;
  std::thread worker_thread_;
  bool stopping_{false};
};

extern "C" {
// C interface for Python ctypes
Machine* CreateMachine();
void DestroyMachine(Machine* machine);
// Enqueue/Dequeue generic tasks; returns 1 on success, 0 on failure/empty.
int PushTaskC(Machine* machine, const Task* task);
int PopTaskC(Machine* machine, Task* task);
}