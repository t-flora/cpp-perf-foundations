#include "check.hpp"

#include <concurrency/thread_pool.hpp>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using concurrency::ThreadPool;
using test::check;

int main() {
    // Results come back through futures
    {
        ThreadPool pool(4);
        std::vector<std::future<long long>> results;
        for (long long i = 0; i < 10'000; ++i) results.push_back(pool.submit([i] { return i * i; }));
        long long sum = 0;
        for (auto& r : results) sum += r.get();
        check(sum == 333'283'335'000LL, "sum of squares over 10k tasks"); // sum i^2, i < 10000
        check(pool.size() == 4, "pool size");
    }

    // Arguments are forwarded, and move-only results work
    {
        ThreadPool pool(2);
        auto f = pool.submit([](int a, int b) { return a + b; }, 40, 2);
        auto p = pool.submit([] { return std::make_unique<int>(7); });
        check(f.get() == 42 && *p.get() == 7, "forwarded arguments and move-only results");
    }

    // An exception thrown in a task is rethrown by future::get() in the caller
    {
        ThreadPool pool(2);
        auto f = pool.submit([]() -> int { throw std::runtime_error("boom"); });
        std::string message;
        try {
            f.get();
        } catch (const std::runtime_error& e) {
            message = e.what();
        }
        check(message == "boom", "exception propagates through the future");
        check(pool.submit([] { return 1; }).get() == 1, "the worker survives a throwing task");
    }

    // The destructor finishes every queued task before joining, even with no get()
    {
        std::atomic<int> done = 0;
        {
            ThreadPool pool(2);
            for (int i = 0; i < 1000; ++i) pool.submit([&done] { ++done; });
        }
        check(done == 1000, "destructor drains the queue");
    }

    // parallel_for visits every index exactly once, including a ragged last chunk
    {
        ThreadPool pool(4);
        std::vector<std::atomic<int>> hits(10'007);
        concurrency::parallel_for(pool, 0, hits.size(), 100, [&](std::size_t i) { ++hits[i]; });
        bool once = true;
        for (const auto& h : hits) once &= h == 1;
        check(once, "parallel_for covers [begin, end) exactly once");

        // An exception from one chunk propagates, after every chunk has finished
        std::atomic<int> finished = 0;
        bool threw = false;
        try {
            concurrency::parallel_for(pool, 0, 1000, 10, [&](std::size_t i) {
                if (i == 500) throw std::logic_error("chunk failed");
                ++finished;
            });
        } catch (const std::logic_error&) {
            threw = true;
        }
        check(threw && finished == 990, "parallel_for rethrows after all other chunks complete");
    }

    // Zero threads is a usage error
    bool rejected = false;
    try {
        ThreadPool pool(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "ThreadPool(0) throws");

    return test::finish("thread_pool");
}
