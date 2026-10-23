// A deliberate data race, to see what goes wrong and what ThreadSanitizer reports.
//
//   cmake -B build-tsan -DDAY6_TSAN=ON && cmake --build build-tsan
//   ./build-tsan/race_demo          # TSan prints "WARNING: ThreadSanitizer: data race"
//   ./build/race_demo               # without TSan: racy is wrong, often exactly 1,000,000
//
// `racy` is a plain int incremented from 4 threads with no synchronization: undefined
// behavior. Even unoptimized, ++racy is load, add, store, and two threads can load the same
// value, so increments get lost. Optimized, it's worse: since a race is UB, the compiler may
// assume no other thread touches `racy` and turn each thread's loop into one
// `racy += 1000000`, and the threads then overwrite each other's single store.
// `safe` uses std::atomic, so every increment lands.
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

int racy = 0;
std::atomic<int> safe = 0;

int main() {
    constexpr int kThreads = 4, kIncrements = 1'000'000;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            for (int i = 0; i < kIncrements; ++i) {
                ++racy;  // data race
                safe.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) t.join();
    std::printf("expected %d\nracy     %d\natomic   %d\n", kThreads * kIncrements, racy, safe.load());
    return 0;
}
