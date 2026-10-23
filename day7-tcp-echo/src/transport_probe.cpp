// Where does a loopback round trip go? Ping-pongs one byte between two threads over a
// pipe, a Unix-domain socket pair, and a TCP connection, each with blocking reads (the
// reader sleeps) and with spinning non-blocking reads (it never sleeps). Then splits one
// TCP hop into the send() call itself and the wait until the other end can read it.
// Prints one JSON object per line.
#include <net/socket.hpp>

#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
constexpr int kRounds = 20'000;

double us(Clock::duration d) { return std::chrono::duration<double, std::micro>(d).count(); }

struct Stats {
    double p50, p99;
};

Stats summarize(std::vector<double>& v) {
    std::sort(v.begin(), v.end());
    return {v[v.size() / 2], v[v.size() * 99 / 100]};
}

void write_byte(int fd) {
    const char c = 'x';
    while (::write(fd, &c, 1) != 1)
        if (errno != EINTR && errno != EAGAIN) net::throw_errno("write");
}

// Blocking fds wait in read(); non-blocking ones spin on EAGAIN
void read_byte(int fd) {
    char c;
    while (::read(fd, &c, 1) != 1)
        if (errno != EINTR && errno != EAGAIN) net::throw_errno("read");
}

// a_* is this thread's end, b_* the echo thread's
Stats ping_pong(int a_read, int a_write, int b_read, int b_write) {
    std::thread echo([=] {
        for (int i = 0; i < kRounds; ++i) {
            read_byte(b_read);
            write_byte(b_write);
        }
    });
    std::vector<double> rtt;
    rtt.reserve(kRounds);
    for (int i = 0; i < kRounds; ++i) {
        const auto t0 = Clock::now();
        write_byte(a_write);
        read_byte(a_read);
        rtt.push_back(us(Clock::now() - t0));
    }
    echo.join();
    return summarize(rtt);
}

void report(const char* transport, const char* wait, Stats s) {
    std::printf("{\"probe\": \"ping_pong\", \"transport\": \"%s\", \"wait\": \"%s\", \"p50_us\": %.2f, \"p99_us\": %.2f}\n",
                transport, wait, s.p50, s.p99);
}

// One direction, one thread: how long send() takes, then how long until the bytes are readable
void one_way(const char* transport, int tx, int rx) {
    std::vector<double> call, delivery;
    for (int i = 0; i < kRounds / 4; ++i) {
        const auto t0 = Clock::now();
        write_byte(tx);
        const auto t1 = Clock::now();
        read_byte(rx); // rx is non-blocking: spins until the byte lands
        const auto t2 = Clock::now();
        call.push_back(us(t1 - t0));
        delivery.push_back(us(t2 - t1));
    }
    const Stats c = summarize(call), d = summarize(delivery);
    std::printf("{\"probe\": \"one_way\", \"transport\": \"%s\", \"send_call_p50_us\": %.2f, \"delivery_p50_us\": %.2f}\n",
                transport, c.p50, d.p50);
}

} // namespace

int main() {
    int p1[2], p2[2], u[2];
    if (::pipe(p1) != 0 || ::pipe(p2) != 0 || ::socketpair(AF_UNIX, SOCK_STREAM, 0, u) != 0) net::throw_errno("pipe");
    const net::Fd pipe_fds[] = {net::Fd(p1[0]), net::Fd(p1[1]), net::Fd(p2[0]), net::Fd(p2[1])};
    const net::Fd unix_a(u[0]), unix_b(u[1]);

    net::Fd listener = net::listen_tcp("127.0.0.1", 0);
    const net::Fd tcp_a = net::connect_tcp("127.0.0.1", net::local_port(listener));
    const net::Fd tcp_b = net::accept_tcp(listener);
    net::set_nodelay(tcp_a, true);
    net::set_nodelay(tcp_b, true);

    for (const bool spin : {false, true}) {
        const char* wait = spin ? "spin" : "block";
        if (spin)
            for (const net::Fd* fd : {&pipe_fds[0], &pipe_fds[2], &unix_a, &unix_b, &tcp_a, &tcp_b})
                net::set_nonblocking(*fd);
        report("pipe", wait, ping_pong(p2[0], p1[1], p1[0], p2[1]));
        report("unix", wait, ping_pong(u[0], u[0], u[1], u[1]));
        report("tcp", wait, ping_pong(tcp_a.get(), tcp_a.get(), tcp_b.get(), tcp_b.get()));
    }
    one_way("unix", u[0], u[1]);
    one_way("tcp", tcp_a.get(), tcp_b.get());
}
