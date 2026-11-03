#pragma once
#include <vector>
#include <atomic>
#include "vfs_types.hpp"

class RingBuffer {
private:
    std::vector<DenseVfsEvent> buffer;
    const size_t capacity;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

public:
    RingBuffer(size_t size) : capacity(size), buffer(size) {}

    bool push(const DenseVfsEvent& event) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % capacity;

        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; 
        }

        buffer[current_tail] = event;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(DenseVfsEvent& event) {
        size_t current_head = head.load(std::memory_order_relaxed);

        if (current_head == tail.load(std::memory_order_acquire)) {
            return false; 
        }

        event = buffer[current_head];
        head.store((current_head + 1) % capacity, std::memory_order_release);
        return true;
    }
};