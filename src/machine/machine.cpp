#include "machine.h"

#include <dlfcn.h>
#include <cstring>
#include <string>

Machine::Machine() {
    worker_thread_ = std::thread(&Machine::WorkerLoop, this);
}

Machine::~Machine() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    dispatch_cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void* Machine::Malloc(size_t size) {
    return std::malloc(size);
}

void Machine::Free(void* ptr) {
    std::free(ptr);
}

void Machine::PushTask(const Task& task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        dispatch_queue_.push(task);
    }
    dispatch_cv_.notify_one();
}

bool Machine::PopTask(Task& task) {
    std::unique_lock<std::mutex> lock(mutex_);
    finish_cv_.wait(lock, [&] { return stopping_ || !finish_queue_.empty(); });
    if (finish_queue_.empty()) {
        return false;
    }
    task = finish_queue_.front();
    finish_queue_.pop();
    return true;
}

void Machine::WorkerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            dispatch_cv_.wait(lock, [&] { return stopping_ || !dispatch_queue_.empty(); });
            if (dispatch_queue_.empty()) {
                return;
            }
            task = dispatch_queue_.front();
            dispatch_queue_.pop();
        }

        // Resolve library path and entry point from caller-managed buffers.
        std::string lib_path;
        if (task.runable.binary && task.runable.binary_length > 0) {
            lib_path.assign(static_cast<const char*>(task.runable.binary),
                            static_cast<size_t>(task.runable.binary_length));
        }
        std::string entry_point = "AddWorker";
        if (task.runable.entry_point && task.runable.entry_length > 0) {
            entry_point.assign(task.runable.entry_point,
                               static_cast<size_t>(task.runable.entry_length));
        }

        if (!lib_path.empty()) {
            void* handle = dlopen(lib_path.c_str(), RTLD_LAZY);
            if (handle) {
                using WorkerFunc = bool (*)(void*, int);
                WorkerFunc fn = reinterpret_cast<WorkerFunc>(dlsym(handle, entry_point.c_str()));
                if (fn) {
                    fn(task.runable.data, task.runable.data_length);
                }
                dlclose(handle);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            finish_queue_.push(task);
        }
        finish_cv_.notify_one();
    }
}

extern "C" Machine* CreateMachine() {
    return new Machine();
}

extern "C" void DestroyMachine(Machine* machine) {
    if (machine) {
        delete machine;
    }
}

extern "C" int PushTaskC(Machine* machine, const Task* task) {
    if (!machine || !task) {
        return 0;
    }
    machine->PushTask(*task);
    return 1;
}

extern "C" int PopTaskC(Machine* machine, Task* task) {
    if (!machine || !task) {
        return 0;
    }
    if (!machine->PopTask(*task)) {
        return 0;
    }
    return 1;
}
