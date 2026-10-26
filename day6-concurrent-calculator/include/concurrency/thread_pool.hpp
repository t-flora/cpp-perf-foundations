#pragma once
// Day 6: a fixed-size thread pool on top of BoundedQueue. Fill in the TODOs.
#include "bounded_queue.hpp"

#include <cstddef>
#include <functional>
#include <future>
#include <thread>
#include <type_traits>
#include <vector>

namespace concurrency {

class ThreadPool {
public:
    // TODO: start `threads` workers that pop tasks from the queue and run them
    explicit ThreadPool(std::size_t threads, std::size_t queue_capacity = 4096) : tasks_(queue_capacity) {
        (void)threads;
    }

    // TODO: stop accepting work, let workers finish everything queued, then join them
    ~ThreadPool() {}

    // TODO: run f(args...) on a worker; the future carries the result or the exception.
    // Hint: std::packaged_task is move-only, but std::function needs a copyable callable
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        (void)f;
        ((void)args, ...);
        return {};
    }

    std::size_t size() const { return workers_.size(); }

private:
    BoundedQueue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
};

// TODO: run body(i) for every i in [begin, end), in chunks of `grain` indices submitted to the
// pool, and wait for all of them. If a chunk throws, what has to happen before you rethrow?
template <typename Body>
void parallel_for(ThreadPool& pool, std::size_t begin, std::size_t end, std::size_t grain, Body body) {
    (void)pool;
    (void)begin;
    (void)end;
    (void)grain;
    (void)body;
}

} // namespace concurrency
