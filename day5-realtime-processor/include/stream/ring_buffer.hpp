#pragma once
#include <bit>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace stream {

// FIFO queue over a contiguous, power-of-two-sized array. Indices wrap with a mask instead
// of a modulo. Unlike std::deque, it allocates once up front, and only again if it fills up:
// it then doubles, and growths() counts how often. Size the capacity so that never happens
// on the hot path.
//
// The interface mirrors the subset of std::deque that Window uses, so the two are
// interchangeable.
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t min_capacity = 1024)
        : buf_(std::bit_ceil(std::max<std::size_t>(min_capacity, 2))), mask_(buf_.size() - 1) {}

    void push_back(const T& value) {
        if (size_ == buf_.size()) grow();
        buf_[(head_ + size_) & mask_] = value;
        ++size_;
    }

    void pop_front() {
        assert(size_ > 0);
        head_ = (head_ + 1) & mask_;
        --size_;
    }

    T& front() { return buf_[head_]; }
    const T& front() const { return buf_[head_]; }

    // i-th element from the front
    const T& operator[](std::size_t i) const { return buf_[(head_ + i) & mask_]; }

    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return buf_.size(); }
    std::size_t growths() const { return growths_; }

private:
    // Unwrap into a buffer twice the size, oldest element first
    void grow() {
        std::vector<T> bigger(buf_.size() * 2);
        for (std::size_t i = 0; i < size_; ++i) bigger[i] = (*this)[i];
        buf_ = std::move(bigger);
        mask_ = buf_.size() - 1;
        head_ = 0;
        ++growths_;
    }

    std::vector<T> buf_;
    std::size_t mask_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::size_t growths_ = 0;
};

} // namespace stream
