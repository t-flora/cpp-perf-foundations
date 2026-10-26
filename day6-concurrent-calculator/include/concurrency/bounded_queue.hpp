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

    // TODO: the number of items right now (it may change as soon as you return it)
    std::size_t size() const { return 0; }
    std::size_t capacity() const { return capacity_; }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_; // mutable: const members like size() lock it too
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> items_;
    bool closed_ = false;
};

} // namespace concurrency
