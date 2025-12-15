#include "ring.h"

std::size_t SpscRingBuffer::AtomicSize::LoadAcquire() const {
  return __sync_fetch_and_add(const_cast<volatile std::size_t*>(&v), 0);
}
void SpscRingBuffer::AtomicSize::StoreRelease(std::size_t x) {
  __sync_lock_test_and_set(&v, x);
}

bool SpscRingBuffer::AtomicBool::LoadAcquire() const {
  return __sync_fetch_and_add(const_cast<volatile int*>(&v), 0) != 0;
}
void SpscRingBuffer::AtomicBool::StoreRelease(bool x) {
  __sync_lock_test_and_set(&v, x ? 1 : 0);
}

SpscRingBuffer::SpscRingBuffer(std::size_t capacity)
    : capacity_(capacity), buffer_(capacity) {}

bool SpscRingBuffer::Push(void* ptr) {
  std::size_t head = head_.LoadAcquire();
  if (head - tail_ >= capacity_) {
    return false; // full
  }
  buffer_[head % capacity_] = ptr;
  head_.StoreRelease(head + 1);
  return true;
}

bool SpscRingBuffer::Get(void*& ptr, std::size_t& idx) {
  std::size_t head = head_.LoadAcquire();
  std::size_t mid = mid_.LoadAcquire();
  if (mid >= head) {
    return false;
  }
  idx = mid % capacity_;
  ptr = buffer_[idx];
  return true;
}

void SpscRingBuffer::Finish(std::size_t idx, void* ptr) {
  buffer_[idx] = ptr;
  mid_.StoreRelease(mid_.LoadAcquire() + 1);
}

bool SpscRingBuffer::Pop(void*& ptr) {
  std::size_t mid = mid_.LoadAcquire();
  if (tail_ >= mid) {
    return false;
  }
  std::size_t idx = tail_ % capacity_;
  ptr = buffer_[idx];
  tail_++;
  return true;
}

void SpscRingBuffer::Stop() { stop_.StoreRelease(true); }
bool SpscRingBuffer::IsStopped() const { return stop_.LoadAcquire(); }

std::size_t SpscRingBuffer::HeadCount() const { return head_.LoadAcquire(); }
std::size_t SpscRingBuffer::MidCount() const { return mid_.LoadAcquire(); }
std::size_t SpscRingBuffer::TailCount() const { return tail_; }
bool SpscRingBuffer::HasReadyForConsumer() const {
  return mid_.LoadAcquire() < head_.LoadAcquire();
}

