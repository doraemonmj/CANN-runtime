#pragma once

#include <cstddef>
#include <vector>

// Single-producer single-consumer ring buffer using three cursors:
// head: next write by producer
// mid: next completion by consumer
// tail: next collection by producer
// Stores opaque void*; producer/consumer负责类型强转与生命周期管理。
class SpscRingBuffer {
public:
  explicit SpscRingBuffer(std::size_t capacity);

  bool Push(void* ptr);                  // producer enqueue
  bool Get(void*& ptr, std::size_t& idx); // consumer fetch pending item
  void Finish(std::size_t idx, void* ptr); // consumer mark done
  bool Pop(void*& ptr);                  // producer collect done

  void Stop();
  bool IsStopped() const;

  std::size_t HeadCount() const;
  std::size_t MidCount() const;
  std::size_t TailCount() const;
  bool HasReadyForConsumer() const;

private:
  struct AtomicSize {
    volatile std::size_t v{0};
    std::size_t LoadAcquire() const;
    void StoreRelease(std::size_t x);
  };
  struct AtomicBool {
    volatile int v{0};
    bool LoadAcquire() const;
    void StoreRelease(bool x);
  };

  const std::size_t capacity_;
  std::vector<void*> buffer_;
  AtomicSize head_;     // next write by producer
  AtomicSize mid_;      // next completion by consumer
  std::size_t tail_{0}; // next collect by producer
  AtomicBool stop_{};
};

