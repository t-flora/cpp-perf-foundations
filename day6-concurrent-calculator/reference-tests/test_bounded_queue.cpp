#include "check.hpp"

#include <concurrency/bounded_queue.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using concurrency::BoundedQueue;
using test::check;
using namespace std::chrono_literals;

namespace {

void single_thread_basics() {
    BoundedQueue<int> q(3);
    check(q.try_push(1) && q.try_push(2) && q.try_push(3), "fills to capacity");
    check(!q.try_push(4), "try_push fails when full");
    check(q.pop() == 1 && q.pop() == 2, "FIFO order");
    check(q.try_pop() == 3 && !q.try_pop(), "try_pop drains, then returns nullopt");
}

// A producer blocks on a full queue until a consumer makes room
void backpressure() {
    BoundedQueue<int> q(2);
    std::atomic<bool> third_pushed = false;
    q.push(1);
    q.push(2);
    std::thread producer([&] {
        q.push(3);
        third_pushed = true;
    });
    std::this_thread::sleep_for(50ms);
    check(!third_pushed, "push blocks while the queue is full");
    check(q.pop() == 1, "consumer pops");
    producer.join();
    check(third_pushed && q.size() == 2, "blocked push completes once there's room");
}

void close_semantics() {
    // Blocked consumer wakes up with nullopt
    BoundedQueue<int> empty(4);
    std::atomic<bool> consumer_done = false;
    std::thread consumer([&] {
        check(!empty.pop().has_value(), "pop on a closed, empty queue returns nullopt");
        consumer_done = true;
    });
    std::this_thread::sleep_for(20ms);
    check(!consumer_done, "consumer is blocked before close()");
    empty.close();
    consumer.join();

    // Blocked producer wakes up with false
    BoundedQueue<int> full(1);
    full.push(1);
    std::thread producer([&] { check(!full.push(2), "push blocked on a full queue fails after close()"); });
    std::this_thread::sleep_for(20ms);
    full.close();
    producer.join();

    // Items queued before close() are still delivered
    BoundedQueue<int> q(4);
    q.push(7);
    q.push(8);
    q.close();
    check(!q.push(9) && !q.try_push(9), "no pushes after close()");
    check(q.pop() == 7 && q.pop() == 8 && !q.pop(), "pops drain remaining items, then nullopt");
}

// 4 producers x 4 consumers through a small queue: every item arrives exactly once
void mpmc_stress() {
    constexpr int kProducers = 4, kConsumers = 4, kPerProducer = 50'000;
    BoundedQueue<int> q(64);
    std::vector<std::vector<int>> received(kConsumers);

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&q, &mine = received[c]] {
            while (auto item = q.pop()) mine.push_back(*item);
        });
    }
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&q, p] {
            for (int i = 0; i < kPerProducer; ++i) q.push(p * kPerProducer + i);
        });
    }
    for (auto& t : producers) t.join();
    q.close(); // only after every producer is done
    for (auto& t : consumers) t.join();

    std::vector<int> seen(kProducers * kPerProducer, 0);
    for (const auto& r : received) {
        for (int v : r) ++seen[v];
    }
    bool exactly_once = true;
    for (int s : seen) exactly_once &= s == 1;
    check(exactly_once, "MPMC: every item delivered exactly once");

    // Per producer, items keep their order (one queue, FIFO)
    bool ordered = true;
    for (const auto& r : received) {
        std::vector<int> last(kProducers, -1);
        for (int v : r) {
            ordered &= v > last[v / kPerProducer];
            last[v / kPerProducer] = v;
        }
    }
    check(ordered, "MPMC: each consumer sees each producer's items in order");
}

} // namespace

int main() {
    single_thread_basics();
    backpressure();
    close_semantics();
    mpmc_stress();
    return test::finish("bounded_queue");
}
