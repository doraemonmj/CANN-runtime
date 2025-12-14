#pragma once

#include <cstddef>

#include "machine_api.h"
#include "platform.h"

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

  platform::Queue<Task> dispatch_queue_;
  platform::Queue<Task> finish_queue_;
  platform::Mutex mutex_;
  platform::ConditionVariable dispatch_cv_;
  platform::ConditionVariable finish_cv_;
  platform::Thread worker_thread_;
  bool stopping_{false};
};
