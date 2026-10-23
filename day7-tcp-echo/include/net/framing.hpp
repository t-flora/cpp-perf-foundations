#pragma once
// Length-prefixed framing: a 4-byte big-endian payload length, then the payload.
// TCP delivers bytes, not messages, so a protocol has to mark where each message ends.
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

inline void put_u32_be(char* p, std::uint32_t v) {
    p[0] = static_cast<char>(v >> 24);
    p[1] = static_cast<char>(v >> 16);
    p[2] = static_cast<char>(v >> 8);
    p[3] = static_cast<char>(v);
}

inline std::uint32_t get_u32_be(const char* p) {
    const auto b = [p](int i) { return static_cast<std::uint32_t>(static_cast<unsigned char>(p[i])); };
    return b(0) << 24 | b(1) << 16 | b(2) << 8 | b(3);
}

inline void append_frame(std::string& out, std::string_view payload) {
    char header[kHeaderSize];
    put_u32_be(header, static_cast<std::uint32_t>(payload.size()));
    out.append(header, kHeaderSize);
    out.append(payload);
}

inline std::string make_frame(std::string_view payload) {
    std::string out;
    out.reserve(kHeaderSize + payload.size());
    append_frame(out, payload);
    return out;
}

// Accumulates bytes as they arrive, in any split, and hands out complete payloads.
class FrameDecoder {
public:
    // Never trust a length from the network: without a limit, one 4-byte header
    // could make us try to buffer 4 GB.
    explicit FrameDecoder(std::uint32_t max_frame = kMaxFrame) : max_frame_(max_frame) {}

    void feed(std::string_view bytes) { buf_.append(bytes); }

    // The next complete payload, or nullopt if more bytes are needed. The view stays
    // valid until the next call to feed() or next().
    std::optional<std::string_view> next() {
        const std::size_t avail = buf_.size() - pos_;
        if (avail >= kHeaderSize) {
            const std::uint32_t len = get_u32_be(buf_.data() + pos_);
            if (len > max_frame_) throw FrameError("frame of " + std::to_string(len) + " bytes exceeds the limit");
            if (avail - kHeaderSize >= len) {
                const std::string_view payload(buf_.data() + pos_ + kHeaderSize, len);
                pos_ += kHeaderSize + len;
                return payload;
            }
        }
        // Incomplete: drop consumed bytes so the buffer doesn't grow forever
        buf_.erase(0, pos_);
        pos_ = 0;
        return std::nullopt;
    }

    // Bytes received but not yet returned as a frame
    std::size_t buffered() const { return buf_.size() - pos_; }

private:
    std::string buf_;
    std::size_t pos_ = 0;
    std::uint32_t max_frame_;
};

} // namespace net
