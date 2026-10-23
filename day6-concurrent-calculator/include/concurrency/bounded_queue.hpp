#pragma once
// Day 6: a multi-producer, multi-consumer blocking queue with a fixed capacity.
// Fill in the TODOs.
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

namespace concurrency {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {}

    // TODO: block while full; return false if the queue is closed
    bool push(T item) {
        (void)item;
        return false;
    }

    // TODO: block while empty and open; return std::nullopt once closed *and* drained
    std::optional<T> pop() { return std::nullopt; }

    // TODO: non-blocking variants
    bool try_push(T item) {
        (void)item;
        return false;
    }
    std::optional<T> try_pop() { return std::nullopt; }

    // TODO: wake every blocked producer and consumer; pushes fail afterwards
    void close() {}

private:
    const std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> items_;
    bool closed_ = false;
};

} // namespace concurrency
