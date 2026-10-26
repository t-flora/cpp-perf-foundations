#include "check.hpp"

#include <net/framing.hpp>

#include <random>
#include <string>
#include <vector>

using test::check;

namespace {

std::vector<std::string> drain(net::FrameDecoder& d) {
    std::vector<std::string> out;
    while (auto f = d.next()) out.emplace_back(*f);
    return out;
}

} // namespace

int main() {
    // Header encoding is big-endian and round-trips
    {
        char h[4];
        net::put_u32_be(h, 0x01020304);
        check(h[0] == 1 && h[1] == 2 && h[2] == 3 && h[3] == 4, "big-endian byte order");
        net::put_u32_be(h, 0xfffffffe);
        check(net::get_u32_be(h) == 0xfffffffe, "high bytes survive (no sign extension)");
    }

    // Several frames in one chunk, including an empty one
    {
        std::string wire = net::make_frame("hello") + net::make_frame("") + net::make_frame("world");
        net::FrameDecoder d;
        d.feed(wire);
        const auto frames = drain(d);
        check(frames == std::vector<std::string>{"hello", "", "world"}, "three frames from one chunk");
        check(d.buffered() == 0, "nothing left over");
    }

    // One byte at a time: nothing until the frame is complete
    {
        const std::string wire = net::make_frame("byte by byte");
        net::FrameDecoder d;
        std::vector<std::string> frames;
        for (std::size_t i = 0; i < wire.size(); ++i) {
            d.feed(wire.substr(i, 1));
            auto got = drain(d);
            check(got.empty() == (i + 1 < wire.size()), "frame appears only with its last byte");
            frames.insert(frames.end(), got.begin(), got.end());
        }
        check(frames == std::vector<std::string>{"byte by byte"}, "byte-by-byte decode");
    }

    // Random payloads cut at random points decode to the same sequence
    {
        std::mt19937 rng(7);
        std::vector<std::string> sent;
        std::string wire;
        for (int i = 0; i < 2000; ++i) {
            std::string p(rng() % 3000, '\0');
            for (auto& c : p) c = static_cast<char>(rng());
            wire += net::make_frame(p);
            sent.push_back(std::move(p));
        }
        net::FrameDecoder d;
        std::vector<std::string> got;
        for (std::size_t pos = 0; pos < wire.size();) {
            const std::size_t n = std::min<std::size_t>(1 + rng() % 5000, wire.size() - pos);
            d.feed(std::string_view(wire).substr(pos, n));
            pos += n;
            auto frames = drain(d);
            got.insert(got.end(), frames.begin(), frames.end());
        }
        check(got == sent, "2000 random frames through random splits");
        check(d.buffered() == 0, "random splits leave nothing buffered");
    }

    // A length over the limit is rejected before any payload is buffered
    {
        net::FrameDecoder d(1024);
        char h[4];
        net::put_u32_be(h, 1025);
        d.feed({h, 4});
        bool threw = false;
        try {
            d.next();
        } catch (const net::FrameError&) {
            threw = true;
        }
        check(threw, "oversized frame throws FrameError");

        net::FrameDecoder ok(1024);
        ok.feed(net::make_frame(std::string(1024, 'a')));
        check(drain(ok).size() == 1, "frame exactly at the limit is accepted");
    }

    return test::finish("test_framing");
}
