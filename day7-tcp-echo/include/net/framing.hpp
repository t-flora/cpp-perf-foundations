#pragma once
// Day 7: length-prefixed framing. A 4-byte big-endian payload length, then the payload.
// Fill in the TODOs.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace net {

inline constexpr std::size_t kHeaderSize = 4;
inline constexpr std::uint32_t kMaxFrame = 16u << 20;

// A peer sent a length we refuse to buffer
struct FrameError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// TODO: big-endian, regardless of the machine's byte order
inline void put_u32_be(char* p, std::uint32_t v) {
    (void)p;
    (void)v;
}
inline std::uint32_t get_u32_be(const char* p) {
    (void)p;
    return 0;
}

// TODO: header + payload
inline std::string make_frame(std::string_view payload) {
    (void)payload;
    return {};
}

// Accepts bytes in any split; hands out complete payloads
class FrameDecoder {
public:
    explicit FrameDecoder(std::uint32_t max_frame = kMaxFrame) : max_frame_(max_frame) {}

    // TODO
    void feed(std::string_view bytes) { (void)bytes; }

    // TODO: the next complete payload, or nullopt if more bytes are needed. Throw
    // FrameError for a length over max_frame_. Don't let the buffer grow forever.
    std::optional<std::string_view> next() {
        (void)max_frame_;
        return std::nullopt;
    }

    // TODO: bytes received but not yet handed out by next()
    std::size_t buffered() const { return 0; }

private:
    std::string buf_;
    std::uint32_t max_frame_;
};

} // namespace net
