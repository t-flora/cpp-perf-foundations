#include "check.hpp"

#include <net/socket.hpp>

#include <csignal>
#include <string>
#include <thread>

using test::check;

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    // Fd closes once, and moving transfers ownership
    {
        int fds[2];
        check(::pipe(fds) == 0, "pipe");
        net::Fd a(fds[0]);
        net::Fd b(std::move(a));
        check(!a && b && b.get() == fds[0], "move leaves the source empty");
        net::Fd c;
        c = std::move(b);
        check(!b && c.get() == fds[0], "move assignment");
        c.reset();
        check(::fcntl(fds[0], F_GETFD) == -1 && errno == EBADF, "reset closes the descriptor");
        ::close(fds[1]);
    }

    // Port 0 gets a real port, and a client can connect to it
    net::Fd listener = net::listen_tcp("127.0.0.1", 0);
    const std::uint16_t port = net::local_port(listener);
    check(port != 0, "OS assigned a port");

    // 8 MB is far more than the socket buffers hold, so send_all must loop over
    // partial sends while the other side reads
    {
        const std::size_t size = 8u << 20;
        std::string data(size, '\0');
        for (std::size_t i = 0; i < size; ++i) data[i] = static_cast<char>(i * 7);

        std::thread sender([&] {
            net::Fd client = net::connect_tcp("127.0.0.1", port);
            net::send_all(client, data);
        }); // client closes when the thread ends

        net::Fd conn = net::accept_tcp(listener);
        std::string got(size, '\0');
        check(net::recv_exact(conn, got), "recv_exact filled 8 MB");
        check(got == data, "8 MB arrived intact");
        sender.join();
        char extra;
        check(net::recv_some(conn, {&extra, 1}) == 0, "recv_some returns 0 after the peer closes");
        check(!net::recv_exact(conn, {&extra, 1}), "recv_exact reports EOF");
    }

    // Sending to a peer that has gone away fails with an error instead of SIGPIPE killing us
    {
        net::Fd client = net::connect_tcp("127.0.0.1", port);
        net::accept_tcp(listener).reset();
        bool threw = false;
        try {
            for (int i = 0; i < 100; ++i) net::send_all(client, std::string(64 * 1024, 'x'));
        } catch (const std::system_error&) {
            threw = true;
        }
        check(threw, "send to a closed peer throws");
    }

    // Nothing listening: connect is refused
    {
        const std::uint16_t closed_port = port;
        listener.reset();
        bool refused = false;
        try {
            net::connect_tcp("127.0.0.1", closed_port);
        } catch (const std::system_error& e) {
            refused = e.code() == std::errc::connection_refused;
        }
        check(refused, "connect to a closed port throws ECONNREFUSED");
    }

    return test::finish("test_socket");
}
