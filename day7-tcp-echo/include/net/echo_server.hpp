#pragma once
// Day 7: an echo server with a thread per connection. Every complete frame goes back
// to its sender. Fill in the TODOs.
#include <net/framing.hpp>
#include <net/socket.hpp>

#include <atomic>
#include <cstddef>
#include <thread>

namespace net {

// poll is the stretch goal: one thread, non-blocking sockets, a poll() loop
enum class ServerMode { threads, poll };

class EchoServer {
public:
    // TODO: start accepting on a background thread
    explicit EchoServer(Fd listener, ServerMode mode = ServerMode::threads) : listener_(std::move(listener)) {
        (void)mode;
    }
    EchoServer(const EchoServer&) = delete;
    EchoServer& operator=(const EchoServer&) = delete;
    ~EchoServer() { stop(); }

    std::uint16_t port() const { return local_port(listener_); }

    // TODO: close every connection and join every thread; safe to call twice.
    // A thread blocked in accept() or recv() won't notice a flag: what wakes it?
    void stop() {}

    // TODO: connections accepted so far
    std::size_t connections_accepted() const { return 0; }

private:
    Fd listener_;
    std::atomic<bool> stopping_{false};
    std::thread loop_;
};

} // namespace net
