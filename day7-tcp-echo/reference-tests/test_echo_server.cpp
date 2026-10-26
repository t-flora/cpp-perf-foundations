#include "check.hpp"

#include <net/echo_server.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <random>
#include <string>
#include <thread>
#include <vector>

using test::check;

namespace {

// Sends `count` frames of random sizes, `depth` at a time, and checks every echo.
// depth × max_size stays well under the socket buffers: this client only reads after
// it has sent, so a server blocked writing to it could otherwise deadlock with it.
bool echo_session(std::uint16_t port, unsigned seed, int count, int depth, std::size_t max_size = 20'000) {
    net::Fd fd = net::connect_tcp("127.0.0.1", port);
    net::set_nodelay(fd, true);
    std::mt19937 rng(seed);
    std::vector<std::string> in_flight;
    net::FrameDecoder decoder;
    std::vector<char> buf(64 * 1024);
    int sent = 0, received = 0;
    std::size_t head = 0;
    while (received < count) {
        while (sent < count && sent - received < depth) {
            std::string p(rng() % max_size, '\0');
            for (auto& c : p) c = static_cast<char>(rng());
            net::send_all(fd, net::make_frame(p));
            in_flight.push_back(std::move(p));
            ++sent;
        }
        const std::size_t n = net::recv_some(fd, buf);
        if (n == 0) return false;
        decoder.feed({buf.data(), n});
        while (auto f = decoder.next()) {
            if (*f != in_flight[head++]) return false;
            ++received;
        }
    }
    return true;
}

void run_suite(net::ServerMode mode, const std::string& name) {
    net::EchoServer server(net::listen_tcp("127.0.0.1", 0), mode);
    const std::uint16_t port = server.port();

    check(echo_session(port, 1, 200, 1), name + ": one client, one request at a time");
    check(echo_session(port, 2, 500, 16, 4000), name + ": one client, 16 requests in flight");

    // Eight clients at once
    {
        std::vector<std::thread> clients;
        std::vector<int> ok(8, 0);
        for (int i = 0; i < 8; ++i)
            clients.emplace_back([&, i] { ok[i] = echo_session(port, 10 + i, 300, 4) ? 1 : 0; });
        for (auto& t : clients) t.join();
        check(std::count(ok.begin(), ok.end(), 1) == 8, name + ": 8 concurrent clients all echoed correctly");
    }

    // A 4 MB frame: larger than the socket buffers in both directions
    {
        net::Fd fd = net::connect_tcp("127.0.0.1", port);
        std::string big(4u << 20, '\0');
        for (std::size_t i = 0; i < big.size(); ++i) big[i] = static_cast<char>(i * 31);
        net::send_all(fd, net::make_frame(big));
        std::string back(net::kHeaderSize + big.size(), '\0');
        check(net::recv_exact(fd, back) && back.substr(net::kHeaderSize) == big, name + ": 4 MB frame echoed");
    }

    // A frame claiming 2 GB gets the client disconnected, and the server keeps serving others
    {
        net::Fd bad = net::connect_tcp("127.0.0.1", port);
        char h[4];
        net::put_u32_be(h, 0x80000000u);
        net::send_all(bad, {h, 4});
        char c;
        bool closed = false;
        try {
            closed = net::recv_some(bad, {&c, 1}) == 0;
        } catch (const std::system_error&) {
            closed = true; // a reset is also a disconnect
        }
        check(closed, name + ": oversized length disconnects the client");
        check(echo_session(port, 3, 50, 1), name + ": still serving after a bad client");
    }

    // stop() returns promptly even with idle clients connected
    {
        net::Fd idle1 = net::connect_tcp("127.0.0.1", port);
        net::Fd idle2 = net::connect_tcp("127.0.0.1", port);
        // Every connection this suite opened, idle ones included
        constexpr std::size_t kConnections = 1 + 1 + 8 + 1 + 1 + 1 + 2;
        // Let the server accept them. A fixed sleep would be flaky on a slow machine or under TSan
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (server.connections_accepted() < kConnections && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const auto t0 = std::chrono::steady_clock::now();
        server.stop();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0);
        check(ms.count() < 1000, name + ": stop() with idle clients took " + std::to_string(ms.count()) + " ms");
        char c;
        bool closed = false;
        try {
            closed = net::recv_some(idle1, {&c, 1}) == 0;
        } catch (const std::system_error&) {
            closed = true;
        }
        check(closed, name + ": stop() closed client connections");
        check(server.connections_accepted() == kConnections, name + ": accepted every connection");
    }
}

} // namespace

// Usage: test_echo_server [threads] [poll]   (default: both)
int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);
    std::vector<std::string> modes(argv + 1, argv + argc);
    if (modes.empty()) modes = {"threads", "poll"};
    for (const auto& m : modes) {
        if (m != "threads" && m != "poll") {
            std::fprintf(stderr, "unknown mode: %s\n", m.c_str());
            return 2;
        }
        run_suite(m == "poll" ? net::ServerMode::poll : net::ServerMode::threads, m);
    }
    return test::finish("test_echo_server");
}
