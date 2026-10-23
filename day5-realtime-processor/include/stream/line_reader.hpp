#pragma once
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

namespace stream {

// Reads a FILE* in large chunks and hands out lines as string_views into its buffer: no
// allocation per line, unlike std::getline into a std::string. A view stays valid only until
// the next call to next(). Works on pipes, so the input can be a live stream.
class LineReader {
public:
    explicit LineReader(std::FILE* file, std::size_t chunk = 1 << 16) : file_(file), buf_(chunk) {}

    bool next(std::string_view& line) {
        while (true) {
            const char* start = buf_.data() + begin_;
            const std::size_t avail = end_ - begin_;
            if (const void* nl = avail ? std::memchr(start, '\n', avail) : nullptr) {
                const auto len = static_cast<std::size_t>(static_cast<const char*>(nl) - start);
                line = {start, len};
                begin_ += len + 1;
                return true;
            }
            if (eof_) {
                if (avail == 0) return false;
                line = {start, avail}; // last line without a trailing newline
                begin_ = end_;
                return true;
            }
            // Move the partial line to the front; grow if one line is longer than the buffer
            std::memmove(buf_.data(), start, avail);
            begin_ = 0;
            end_ = avail;
            if (end_ == buf_.size()) buf_.resize(buf_.size() * 2);
            const std::size_t n = std::fread(buf_.data() + end_, 1, buf_.size() - end_, file_);
            if (n == 0) eof_ = true;
            end_ += n;
        }
    }

private:
    std::FILE* file_;
    std::vector<char> buf_;
    std::size_t begin_ = 0;
    std::size_t end_ = 0;
    bool eof_ = false;
};

} // namespace stream
