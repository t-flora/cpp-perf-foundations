#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace concurrency {

// Multi-producer, multi-consumer blocking queue with a fixed capacity.
//
// - push() blocks while the queue is full. That's backpressure: a fast producer slows to the
//   consumers' pace instead of growing memory without bound.
// - pop() blocks while the queue is empty.
// - close() is the shutdown signal. Pushes fail from then on, while pops drain what's left
//   and then return std::nullopt, so consumers can loop on `while (auto item = q.pop())`.
template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    // Blocks while full. Returns false (and drops the item) if the queue is closed
    bool push(T item) {
        {
            std::unique_lock lock(mutex_);
            // The predicate form re-checks after every wakeup, which handles spurious wakeups
            not_full_.wait(lock, [&] { return closed_ || items_.size() < capacity_; });
            if (closed_) return false;
            items_.push_back(std::move(item));
        }
        // Notify after unlocking, so the woken thread doesn't immediately block on the mutex
        not_empty_.notify_one();
        return true;
    }

    // Blocks while empty and open. Returns std::nullopt once closed *and* drained
    std::optional<T> pop() {
        std::optional<T> item;
        {
            std::unique_lock lock(mutex_);
            not_empty_.wait(lock, [&] { return closed_ || !items_.empty(); });
            if (items_.empty()) return std::nullopt; // closed and drained
            item.emplace(std::move(items_.front()));
            items_.pop_front();
        }
        not_full_.notify_one();
        return item;
    }

    // Non-blocking variants
    bool try_push(T item) {
        {
            std::lock_guard lock(mutex_);
            if (closed_ || items_.size() >= capacity_) return false;
            items_.push_back(std::move(item));
        }
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> try_pop() {
        std::optional<T> item;
        {
            std::lock_guard lock(mutex_);
            if (items_.empty()) return std::nullopt;
            item.emplace(std::move(items_.front()));
            items_.pop_front();
        }
        not_full_.notify_one();
        return item;
    }

    // Wakes every blocked producer and consumer. Idempotent
    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    bool closed() const {
        std::lock_guard lock(mutex_);
        return closed_;
    }

    std::size_t size() const {
        std::lock_guard lock(mutex_);
        return items_.size();
    }

    std::size_t capacity() const { return capacity_; }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> items_;
    bool closed_ = false;
};

} // namespace concurrency
