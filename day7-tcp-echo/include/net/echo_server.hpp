#pragma once
// Echoes every complete frame back to its sender. Two designs:
//   threads: one thread per connection, blocking I/O. Simple, one stack per client.
//   poll:    one thread for all connections, non-blocking I/O, readiness from poll().
// Both decode with a FrameDecoder and answer every frame that arrived in one read
// with a single send.
#include <net/framing.hpp>
#include <net/socket.hpp>

#include <poll.h>

#include <atomic>
#include <list>
#include <memory>
#include <thread>
#include <vector>

namespace net {

enum class ServerMode { threads, poll };

class EchoServer {
public:
    // Starts serving immediately on a background thread
    EchoServer(Fd listener, ServerMode mode) : listener_(std::move(listener)), mode_(mode) {
        int fds[2];
        if (::pipe(fds) != 0) throw_errno("pipe");
        wake_read_ = Fd(fds[0]);
        wake_write_ = Fd(fds[1]);
        set_nonblocking(listener_); // poll() can report a connection that's gone by accept()
        loop_ = std::thread([this] { mode_ == ServerMode::threads ? run_threads() : run_poll(); });
    }
    EchoServer(const EchoServer&) = delete;
    EchoServer& operator=(const EchoServer&) = delete;
    ~EchoServer() { stop(); }

    std::uint16_t port() const { return local_port(listener_); }
    std::size_t connections_accepted() const { return accepted_.load(); }

    // Closes every connection and joins every thread. Safe to call more than once.
    void stop() {
        if (stopping_.exchange(true)) return;
        const char byte = 1;
        [[maybe_unused]] const auto n = ::write(wake_write_.get(), &byte, 1); // the self-pipe trick
        loop_.join();
    }

private:
    static constexpr std::size_t kReadSize = 64 * 1024;
    static constexpr std::size_t kMaxPendingOutput = 1 << 20;

    // Sets a new connection's options. Fails if the client already reset it (macOS then
    // rejects setsockopt with EINVAL); that connection is dropped, not the server.
    static bool configure(const Fd& fd, bool nonblocking) {
        try {
            set_nonblocking(fd, nonblocking); // explicit: macOS inherits the listener's flag
            set_nodelay(fd, true);
            return true;
        } catch (const std::system_error&) {
            return false;
        }
    }

    // Blocks until the listener or the wake pipe is readable. False means stop.
    bool wait_for_listener() {
        pollfd fds[2] = {{wake_read_.get(), POLLIN, 0}, {listener_.get(), POLLIN, 0}};
        while (::poll(fds, 2, -1) < 0)
            if (errno != EINTR) throw_errno("poll");
        return fds[0].revents == 0;
    }

    // --- threads ---
    struct Connection {
        Fd fd;
        std::thread thread;
        std::atomic<bool> done{false};
    };

    void run_threads() {
        // Only this thread touches the list, so it needs no lock
        std::list<std::unique_ptr<Connection>> conns;
        while (wait_for_listener()) {
            while (Fd fd = accept_tcp(listener_)) {
                ++accepted_;
                if (!configure(fd, /*nonblocking=*/false)) continue;
                // Join threads whose clients left, so they don't pile up
                conns.remove_if([](const auto& c) {
                    if (!c->done) return false;
                    c->thread.join();
                    return true;
                });
                auto& c = *conns.emplace_back(std::make_unique<Connection>());
                c.fd = std::move(fd);
                c.thread = std::thread([&c] {
                    serve_blocking(c.fd);
                    // Tell the client now. close() waits for the join: closing here could let
                    // the number be reused by a new socket before stop() calls shutdown() on it
                    ::shutdown(c.fd.get(), SHUT_RDWR);
                    c.done = true;
                });
            }
        }
        // shutdown() wakes a thread blocked in recv() or send() on that socket
        for (auto& c : conns) ::shutdown(c->fd.get(), SHUT_RDWR);
        for (auto& c : conns) c->thread.join();
    }

    static void serve_blocking(const Fd& fd) {
        FrameDecoder decoder;
        std::string out;
        std::vector<char> buf(kReadSize);
        try {
            while (const std::size_t n = recv_some(fd, buf)) {
                decoder.feed({buf.data(), n});
                out.clear();
                while (auto frame = decoder.next()) append_frame(out, *frame);
                if (!out.empty()) send_all(fd, out);
            }
        } catch (const FrameError&) {
            // Bad length: drop the client
        } catch (const std::system_error&) {
            // Reset by the peer, or shut down by stop()
        }
    }

    // --- poll ---
    struct PollConnection {
        Fd fd;
        FrameDecoder decoder;
        std::string out;
        std::size_t sent = 0; // bytes of `out` already written
        bool closed = false;

        std::size_t pending() const { return out.size() - sent; }
    };

    void run_poll() {
        // fds[0] is the wake pipe, fds[1] the listener, fds[i + 2] belongs to conns[i]
        std::vector<pollfd> fds = {{wake_read_.get(), POLLIN, 0}, {listener_.get(), POLLIN, 0}};
        std::vector<PollConnection> conns;
        std::vector<char> buf(kReadSize);

        for (;;) {
            while (::poll(fds.data(), fds.size(), -1) < 0)
                if (errno != EINTR) throw_errno("poll");
            if (fds[0].revents != 0) break;

            for (std::size_t i = 0; i < conns.size(); ++i) {
                pollfd& p = fds[i + 2];
                PollConnection& c = conns[i];
                if (p.revents == 0) continue;
                if (p.revents & (POLLIN | POLLHUP | POLLERR)) read_available(c, buf);
                if (!c.closed && c.pending() > 0) flush(c);
                // Backpressure: a client that sends faster than it reads stops being read
                // until its responses drain. Its output buffer stays bounded.
                p.events = static_cast<short>((c.pending() > 0 ? POLLOUT : 0) |
                                              (c.pending() < kMaxPendingOutput ? POLLIN : 0));
            }

            if (fds[1].revents & POLLIN) {
                while (Fd fd = accept_tcp(listener_)) {
                    ++accepted_;
                    if (!configure(fd, /*nonblocking=*/true)) continue;
                    fds.push_back({fd.get(), POLLIN, 0});
                    conns.push_back({std::move(fd), FrameDecoder{}, {}, 0, false});
                }
            }

            // Swap-and-pop closed connections, keeping fds and conns aligned
            for (std::size_t i = conns.size(); i-- > 0;) {
                if (!conns[i].closed) continue;
                std::swap(conns[i], conns.back());
                std::swap(fds[i + 2], fds.back());
                conns.pop_back();
                fds.pop_back();
            }
        }
    }

    // Reads until the socket would block, decoding frames into the output buffer
    static void read_available(PollConnection& c, std::vector<char>& buf) {
        while (c.pending() < kMaxPendingOutput) {
            const ssize_t n = ::recv(c.fd.get(), buf.data(), buf.size(), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            if (n <= 0) { // 0 is EOF; anything else is a reset or another error
                c.closed = true;
                return;
            }
            c.decoder.feed({buf.data(), static_cast<std::size_t>(n)});
            try {
                while (auto frame = c.decoder.next()) append_frame(c.out, *frame);
            } catch (const FrameError&) {
                c.closed = true;
                return;
            }
        }
    }

    // Writes as much as the kernel accepts without blocking
    static void flush(PollConnection& c) {
        while (c.pending() > 0) {
            const ssize_t n = ::send(c.fd.get(), c.out.data() + c.sent, c.pending(), detail::kSendFlags);
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            if (n < 0) {
                c.closed = true;
                return;
            }
            c.sent += static_cast<std::size_t>(n);
        }
        if (c.sent == c.out.size()) {
            c.out.clear();
            c.sent = 0;
        }
    }

    Fd listener_;
    ServerMode mode_;
    Fd wake_read_, wake_write_;
    std::atomic<std::size_t> accepted_{0};
    std::atomic<bool> stopping_{false};
    std::thread loop_; // last: starts after everything it uses exists
};

} // namespace net
