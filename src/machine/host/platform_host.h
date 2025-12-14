#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>

namespace platform {
using Mutex = std::mutex;
using ConditionVariable = std::condition_variable;
using Thread = std::thread;

template <typename T>
using Queue = std::queue<T>;

void* Alloc(std::size_t size);
void Free(void* ptr);
}  // namespace platform

