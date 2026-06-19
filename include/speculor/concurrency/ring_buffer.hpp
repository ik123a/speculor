#pragma once
#include <vector>
#include <cstddef>
#include <memory>
#include <atomic>

namespace speculor {

// Simple lock-free SPMC ring buffer for event ingestion
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) : capacity_(capacity) {
        buffer_ = std::make_unique<Item[]>(capacity_);
    }
    ~RingBuffer() = default;

    // Producer: push an item (single producer)
    bool try_push(const T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        if (tail - head >= capacity_) {
            return false; // Full
        }
        size_t index = tail % capacity_;
        if (buffer_[index].ready.load(std::memory_order_relaxed)) {
            return false; // Slot not yet read by consumer
        }
        buffer_[index].data = item;
        buffer_[index].ready.store(true, std::memory_order_release);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool try_push(T&& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        if (tail - head >= capacity_) {
            return false; // Full
        }
        size_t index = tail % capacity_;
        if (buffer_[index].ready.load(std::memory_order_relaxed)) {
            return false; // Slot not yet read by consumer
        }
        buffer_[index].data = std::move(item);
        buffer_[index].ready.store(true, std::memory_order_release);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // Consumer: pop an item (multiple consumers)
    bool try_pop(T& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        while (true) {
            size_t tail = tail_.load(std::memory_order_acquire);
            if (head == tail) {
                return false; // Empty
            }
            size_t index = head % capacity_;
            if (!buffer_[index].ready.load(std::memory_order_acquire)) {
                return false; // Item not yet ready (race condition or producer writing)
            }
            if (head_.compare_exchange_weak(head, head + 1, std::memory_order_acq_rel)) {
                item = std::move(buffer_[index].data);
                buffer_[index].ready.store(false, std::memory_order_release);
                return true;
            }
        }
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    bool full() const {
        return tail_.load(std::memory_order_relaxed) - head_.load(std::memory_order_relaxed) >= capacity_;
    }

    size_t size() const {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    size_t capacity() const {
        return capacity_;
    }

    void clear() {
        T dummy;
        while (try_pop(dummy)) {}
    }

private:
    struct alignas(64) Item {
        T data;
        std::atomic<bool> ready{false};
    };

    std::unique_ptr<Item[]> buffer_;
    size_t capacity_{0};
    alignas(64) std::atomic<size_t> head_{0};  // read index
    alignas(64) std::atomic<size_t> tail_{0};  // write index
};

} // namespace speculor