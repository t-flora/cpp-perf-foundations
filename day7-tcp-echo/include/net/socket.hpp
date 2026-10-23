#pragma once
// Day 7: a thin RAII layer over POSIX TCP sockets (IPv4). Errors throw std::system_error
// carrying errno. Fill in the TODOs.
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

// Owns a file descriptor and closes it exactly once
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) noexcept : fd_(fd) {}
    // TODO: move-only. What must a moved-from Fd hold?
    ~Fd() { reset(); }

    int get() const noexcept { return fd_; }
    explicit operator bool() const noexcept { return fd_ >= 0; }
    void reset() noexcept {} // TODO

private:
    int fd_ = -1;
};

// TODO: TCP_NODELAY on or off
inline void set_nodelay(const Fd& fd, bool on) {
    (void)fd;
    (void)on;
}

// TODO: socket, SO_REUSEADDR (why?), bind, listen. Port 0 picks any free port.
inline Fd listen_tcp(const std::string& host, std::uint16_t port, int backlog = 128) {
    (void)host;
    (void)port;
    (void)backlog;
    return Fd{};
}

// TODO: getsockname
inline std::uint16_t local_port(const Fd& fd) {
    (void)fd;
    return 0;
}

inline Fd connect_tcp(const std::string& host, std::uint16_t port) {
    (void)host;
    (void)port;
    return Fd{};
}

inline Fd accept_tcp(const Fd& listener) {
    (void)listener;
    return Fd{};
}

// TODO: loop until every byte is written. What about EINTR, and SIGPIPE?
inline void send_all(const Fd& fd, std::string_view data) {
    (void)fd;
    (void)data;
}

// TODO: whatever has arrived, up to buf.size() bytes; 0 means the peer closed
inline std::size_t recv_some(const Fd& fd, std::span<char> buf) {
    (void)fd;
    (void)buf;
    return 0;
}

// TODO: fill buf completely; false if the peer closed first
inline bool recv_exact(const Fd& fd, std::span<char> buf) {
    (void)fd;
    (void)buf;
    return false;
}

} // namespace net
