#pragma once
// Thin RAII layer over POSIX TCP sockets (macOS and Linux, IPv4). Errors throw
// std::system_error carrying errno; EINTR is retried everywhere.
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace net {

[[noreturn]] inline void throw_errno(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}

// Owns a file descriptor and closes it exactly once. Move-only, like unique_ptr.
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) noexcept : fd_(fd) {}
    Fd(Fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    ~Fd() { reset(); }

    int get() const noexcept { return fd_; }
    explicit operator bool() const noexcept { return fd_ >= 0; }
    void reset() noexcept {
        if (fd_ >= 0) ::close(fd_);
        fd_ = -1;
    }

private:
    int fd_ = -1;
};

inline void set_option(const Fd& fd, int level, int name, int value, const char* what) {
    if (::setsockopt(fd.get(), level, name, &value, sizeof value) != 0) throw_errno(what);
}

// Nagle's algorithm holds back small writes while earlier data is unacknowledged.
// TCP_NODELAY turns it off, so every send() goes out immediately.
inline void set_nodelay(const Fd& fd, bool on) { set_option(fd, IPPROTO_TCP, TCP_NODELAY, on ? 1 : 0, "TCP_NODELAY"); }

// Non-blocking calls fail with EAGAIN instead of waiting. On macOS and the BSDs, a socket
// returned by accept() inherits this flag from the listener; on Linux it doesn't.
inline void set_nonblocking(const Fd& fd, bool on = true) {
    const int flags = ::fcntl(fd.get(), F_GETFL);
    if (flags < 0 || ::fcntl(fd.get(), F_SETFL, on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
        throw_errno("O_NONBLOCK");
}

namespace detail {

// Writing to a socket the peer has closed raises SIGPIPE, which kills the process by
// default. macOS can disable it per socket; Linux per send() (MSG_NOSIGNAL).
inline void no_sigpipe([[maybe_unused]] const Fd& fd) {
#ifdef SO_NOSIGPIPE
    set_option(fd, SOL_SOCKET, SO_NOSIGPIPE, 1, "SO_NOSIGPIPE");
#endif
}

#ifdef MSG_NOSIGNAL
inline constexpr int kSendFlags = MSG_NOSIGNAL;
#else
inline constexpr int kSendFlags = 0;
#endif

inline sockaddr_in make_addr(const std::string& host, std::uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        throw std::system_error(std::make_error_code(std::errc::invalid_argument), "bad IPv4 address: " + host);
    return addr;
}

inline Fd new_tcp_socket() {
    Fd fd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!fd) throw_errno("socket");
    no_sigpipe(fd);
    return fd;
}

} // namespace detail

// Port 0 asks the OS for any free port; read it back with local_port().
// Bind to 127.0.0.1 unless you mean to accept connections from other machines.
inline Fd listen_tcp(const std::string& host, std::uint16_t port, int backlog = 128) {
    Fd fd = detail::new_tcp_socket();
    // Lets a restarted server rebind while old connections sit in TIME_WAIT
    set_option(fd, SOL_SOCKET, SO_REUSEADDR, 1, "SO_REUSEADDR");
    const sockaddr_in addr = detail::make_addr(host, port);
    if (::bind(fd.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) != 0) throw_errno("bind");
    if (::listen(fd.get(), backlog) != 0) throw_errno("listen");
    return fd;
}

inline std::uint16_t local_port(const Fd& fd) {
    sockaddr_in addr{};
    socklen_t len = sizeof addr;
    if (::getsockname(fd.get(), reinterpret_cast<sockaddr*>(&addr), &len) != 0) throw_errno("getsockname");
    return ntohs(addr.sin_port);
}

inline Fd connect_tcp(const std::string& host, std::uint16_t port) {
    Fd fd = detail::new_tcp_socket();
    const sockaddr_in addr = detail::make_addr(host, port);
    while (::connect(fd.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) != 0)
        if (errno != EINTR) throw_errno("connect");
    return fd;
}

// Returns an empty Fd if a non-blocking listener has nothing pending
inline Fd accept_tcp(const Fd& listener) {
    for (;;) {
        const int fd = ::accept(listener.get(), nullptr, nullptr);
        if (fd >= 0) {
            Fd conn(fd);
            detail::no_sigpipe(conn);
            return conn;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) return Fd{};
        // The client gave up between SYN and accept(): not the listener's problem
        if (errno != EINTR && errno != ECONNABORTED) throw_errno("accept");
    }
}

// send() may write only part of the buffer (the kernel's send buffer filled up), so
// loop until everything is written. Blocking sockets only.
inline void send_all(const Fd& fd, std::string_view data) {
    while (!data.empty()) {
        const ssize_t n = ::send(fd.get(), data.data(), data.size(), detail::kSendFlags);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("send");
        }
        data.remove_prefix(static_cast<std::size_t>(n));
    }
}

// Whatever has arrived, up to buf.size() bytes. 0 means the peer closed the connection.
// TCP is a byte stream: one send() on the other side can arrive as several recv()s,
// and several sends can arrive as one.
inline std::size_t recv_some(const Fd& fd, std::span<char> buf) {
    for (;;) {
        const ssize_t n = ::recv(fd.get(), buf.data(), buf.size(), 0);
        if (n >= 0) return static_cast<std::size_t>(n);
        if (errno != EINTR) throw_errno("recv");
    }
}

// Fills buf completely. Returns false if the peer closed the connection first.
inline bool recv_exact(const Fd& fd, std::span<char> buf) {
    while (!buf.empty()) {
        const std::size_t n = recv_some(fd, buf);
        if (n == 0) return false;
        buf = buf.subspan(n);
    }
    return true;
}

} // namespace net
