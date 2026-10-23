#pragma once
#include "bounded_queue.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace concurrency {

// Fixed-size thread pool over a BoundedQueue of tasks.
//
// submit() returns a std::future for the task's result, and an exception thrown by the task
// is rethrown by future::get() in the caller. The destructor stops accepting work, lets the
// workers finish everything already queued, then joins them.
//
// Deadlock warning: a task that submits another task and then waits on its future can
// deadlock once every worker is doing the same. Don't block on pool futures from inside
// pool tasks.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads = default_threads(), std::size_t queue_capacity = 4096)
        : tasks_(queue_capacity) {
        if (threads == 0) throw std::invalid_argument("ThreadPool needs at least one thread");
        workers_.reserve(threads);
        for (std::size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        tasks_.close(); // workers drain the queue, then pop() returns nullopt and they exit
        for (auto& w : workers_) w.join();
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task is move-only, but std::function needs a copyable callable, so the
        // task lives in a shared_ptr. (C++23's std::move_only_function removes the need.)
        auto task = std::make_shared<std::packaged_task<R()>>(
            [fn = std::forward<F>(f), ... as = std::forward<Args>(args)]() mutable { return std::invoke(fn, as...); });
        std::future<R> result = task->get_future();
        if (!tasks_.push([task] { (*task)(); })) throw std::runtime_error("submit() on a stopped ThreadPool");
        return result;
    }

    std::size_t size() const { return workers_.size(); }

    static std::size_t default_threads() { return std::max(1u, std::thread::hardware_concurrency()); }

private:
    void worker_loop() {
        while (auto task = tasks_.pop()) (*task)();
    }

    BoundedQueue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_; // declared last: started after the queue exists
};

// Runs body(i) for every i in [begin, end), split into chunks of `grain` submitted to the
// pool, and waits for all of them. Small grains balance load better; large grains cost less
// in queue traffic. If chunks throw, the first exception is rethrown, but only after *every*
// chunk has finished: they all reference `body`, which must outlive them.
template <typename Body>
void parallel_for(ThreadPool& pool, std::size_t begin, std::size_t end, std::size_t grain, Body body) {
    if (grain == 0) grain = 1;
    std::vector<std::future<void>> chunks;
    chunks.reserve((end - begin + grain - 1) / grain);
    for (std::size_t lo = begin; lo < end; lo += grain) {
        const std::size_t hi = std::min(end, lo + grain);
        chunks.push_back(pool.submit([lo, hi, &body] {
            for (std::size_t i = lo; i < hi; ++i) body(i);
        }));
    }
    std::exception_ptr first_error;
    for (auto& c : chunks) {
        try {
            c.get();
        } catch (...) {
            if (!first_error) first_error = std::current_exception();
        }
    }
    if (first_error) std::rethrow_exception(first_error);
}

} // namespace concurrency
