#pragma once
// Day 5: a FIFO over a contiguous, power-of-two-sized array. Fill in the TODOs.
#include <cstddef>
#include <vector>

namespace stream {

template <typename T>
class RingBuffer {
public:
    // TODO: round min_capacity up to a power of two, and allocate once
    explicit RingBuffer(std::size_t min_capacity = 1024) { (void)min_capacity; }

    // TODO: what happens when it's full? Make that event visible (e.g. count it).
    // The reference tests expect it to double, counting each time in growths()
    void push_back(const T& value) { (void)value; }
    void pop_front() {}
    T& front() { return buf_[0]; }
    const T& front() const { return buf_[0]; }
    // TODO: the i-th element from the front
    T& operator[](std::size_t i) { return buf_[i]; }
    const T& operator[](std::size_t i) const { return buf_[i]; }
    bool empty() const { return true; }
    std::size_t size() const { return 0; }
    std::size_t capacity() const { return buf_.size(); }
    std::size_t growths() const { return 0; }

private:
    std::vector<T> buf_;
    // TODO: head index, size, mask (capacity - 1)
};

} // namespace stream
