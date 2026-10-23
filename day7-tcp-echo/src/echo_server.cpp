// echo_server [--host 127.0.0.1] [--port 0] [--mode threads|poll]
// Prints the port it listens on (useful with --port 0), then serves until SIGINT/SIGTERM.
#include <net/echo_server.hpp>

#include <pthread.h>

#include <csignal>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::uint16_t port = 0;
    net::ServerMode mode = net::ServerMode::threads;
    for (int i = 1; i < argc; i += 2) {
        const std::string a = argv[i], v = i + 1 < argc ? argv[i + 1] : "";
        if (a == "--host" && !v.empty()) host = v;
        else if (a == "--port" && !v.empty()) port = static_cast<std::uint16_t>(std::stoi(v));
        else if (a == "--mode" && (v == "threads" || v == "poll"))
            mode = v == "threads" ? net::ServerMode::threads : net::ServerMode::poll;
        else {
            std::cerr << "usage: echo_server [--host H] [--port P] [--mode threads|poll]\n";
            return 2;
        }
    }

    // Block the shutdown signals in every thread (threads inherit the mask), then wait
    // for them synchronously here instead of in an async signal handler
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);
    std::signal(SIGPIPE, SIG_IGN);

    try {
        net::EchoServer server(net::listen_tcp(host, port), mode);
        std::cout << "listening on " << host << ":" << server.port() << std::endl;
        int sig = 0;
        sigwait(&signals, &sig);
        server.stop();
        std::cerr << "accepted " << server.connections_accepted() << " connections\n";
    } catch (const std::exception& e) {
        std::cerr << "echo_server: " << e.what() << "\n";
        return 1;
    }
}
