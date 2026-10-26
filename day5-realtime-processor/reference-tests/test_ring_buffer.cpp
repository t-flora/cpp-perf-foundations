#include "check.hpp"

#include <stream/ring_buffer.hpp>

#include <deque>
#include <random>

using stream::RingBuffer;
using test::check;

int main() {
    // Capacity rounds up to a power of two
    check(RingBuffer<int>(1000).capacity() == 1024, "capacity rounds up to 1024");
    check(RingBuffer<int>(1024).capacity() == 1024, "power-of-two capacity is kept");
    check(RingBuffer<int>(0).capacity() >= 2, "minimum capacity");

    // FIFO order across many wrap-arounds, without growing
    RingBuffer<int> rb(8);
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 100; ++round) {
        for (int i = 0; i < 5; ++i) rb.push_back(next_in++);
        for (int i = 0; i < 5; ++i) {
            check(rb.front() == next_out, "FIFO order across wrap-around");
            rb.pop_front();
            ++next_out;
        }
    }
    check(rb.empty() && rb.growths() == 0, "no growth when size stays within capacity");

    // Growing while wrapped keeps order and indexing intact
    RingBuffer<int> g(4);
    g.push_back(0); g.push_back(1); g.push_back(2);
    g.pop_front(); g.pop_front();                  // head is now mid-buffer
    for (int i = 3; i < 20; ++i) g.push_back(i);   // 18 elements: grows 4 -> 8 -> 16 -> 32
    check(g.growths() == 3 && g.capacity() == 32, "doubles when full, starting while wrapped");
    check(g.size() == 18, "size after growth");
    bool indexed = true;
    for (std::size_t i = 0; i < g.size(); ++i) indexed &= g[i] == static_cast<int>(i) + 2;
    check(indexed, "operator[] in FIFO order after growth");

    // Randomized comparison against std::deque
    RingBuffer<int> r(16);
    std::deque<int> d;
    std::mt19937 rng(1);
    bool same = true;
    for (int step = 0; step < 100000; ++step) {
        if (d.empty() || rng() % 3 != 0) {
            const int v = static_cast<int>(rng());
            r.push_back(v);
            d.push_back(v);
        } else {
            same &= r.front() == d.front();
            r.pop_front();
            d.pop_front();
        }
        same &= r.size() == d.size();
    }
    check(same, "matches std::deque over 100k random operations");

    return test::finish("ring_buffer");
}
