// Load generator for the echo server. Each connection runs on its own thread and keeps
// up to --depth requests in flight, timing every round trip. Prints one JSON object.
//
//   echo_client --port 5555 [--host 127.0.0.1] [--connections 1] [--depth 1] [--size 64]
//               [--messages 10000] [--warmup 1000] [--nagle] [--split] [--label name]
//
// --nagle leaves Nagle's algorithm on (the default here is TCP_NODELAY).
// --split sends each request's header and payload with two send() calls.
#include <net/framing.hpp>
#include <net/socket.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <latch>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    std::string host = "127.0.0.1";
    std::uint16_t port = 0;
    int connections = 1;
    int depth = 1;
    std::size_t size = 64;
    long messages = 10'000; // per connection, including warmup
    long warmup = 1'000;
    bool nagle = false;
    bool split = false;
    std::string label;
};

struct ConnectionResult {
    std::vector<std::int64_t> rtt_ns; // measured requests only
    Clock::time_point end;
    std::string error;
};

// Request n carries n in its first 8 bytes, so the client can check responses come
// back complete and in order
void stamp(std::string& payload, std::uint64_t seq) {
    if (payload.size() >= sizeof seq) std::memcpy(payload.data(), &seq, sizeof seq);
}

void run_connection(const Options& opt, std::latch& ready, ConnectionResult& result) {
    try {
        net::Fd fd = net::connect_tcp(opt.host, opt.port);
        net::set_nodelay(fd, !opt.nagle);

        std::string payload(opt.size, 'x');
        std::string frame;
        std::vector<Clock::time_point> sent_at(static_cast<std::size_t>(opt.depth));
        net::FrameDecoder decoder;
        std::vector<char> buf(64 * 1024);
        result.rtt_ns.reserve(static_cast<std::size_t>(std::max(0L, opt.messages - opt.warmup)));

        long sent = 0;
        long received = 0;
        const auto send_next = [&] {
            stamp(payload, static_cast<std::uint64_t>(sent));
            sent_at[static_cast<std::size_t>(sent % opt.depth)] = Clock::now();
            if (opt.split) {
                char header[net::kHeaderSize];
                net::put_u32_be(header, static_cast<std::uint32_t>(payload.size()));
                net::send_all(fd, {header, net::kHeaderSize});
                net::send_all(fd, payload);
            } else {
                frame.clear();
                net::append_frame(frame, payload);
                net::send_all(fd, frame);
            }
            ++sent;
        };

        ready.arrive_and_wait(); // every connection is open: start together
        while (sent < std::min<long>(opt.depth, opt.messages)) send_next();

        while (received < opt.messages) {
            const std::size_t n = net::recv_some(fd, buf);
            if (n == 0) throw std::runtime_error("server closed the connection");
            decoder.feed({buf.data(), n});
            while (auto response = decoder.next()) {
                const auto now = Clock::now();
                std::uint64_t seq = static_cast<std::uint64_t>(received);
                if (response->size() >= sizeof seq) std::memcpy(&seq, response->data(), sizeof seq);
                if (response->size() != opt.size || seq != static_cast<std::uint64_t>(received))
                    throw std::runtime_error("bad response " + std::to_string(received));
                if (received >= opt.warmup) {
                    const auto rtt = now - sent_at[static_cast<std::size_t>(received % opt.depth)];
                    result.rtt_ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(rtt).count());
                }
                ++received;
                if (sent < opt.messages) send_next();
            }
        }
        result.end = Clock::now();
    } catch (const std::exception& e) {
        result.error = e.what();
        result.end = Clock::now();
    }
}

double percentile_us(std::vector<std::int64_t>& v, double q) {
    if (v.empty()) return 0;
    const auto k = static_cast<std::size_t>(q * static_cast<double>(v.size() - 1));
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k), v.end());
    return static_cast<double>(v[k]) / 1e3;
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(a + " needs a value");
            return argv[++i];
        };
        if (a == "--host") o.host = value();
        else if (a == "--port") o.port = static_cast<std::uint16_t>(std::stoi(value()));
        else if (a == "--connections") o.connections = std::stoi(value());
        else if (a == "--depth") o.depth = std::stoi(value());
        else if (a == "--size") o.size = std::stoul(value());
        else if (a == "--messages") o.messages = std::stol(value());
        else if (a == "--warmup") o.warmup = std::stol(value());
        else if (a == "--nagle") o.nagle = true;
        else if (a == "--split") o.split = true;
        else if (a == "--label") o.label = value();
        else throw std::invalid_argument("unknown option " + a);
    }
    if (o.port == 0 || o.connections < 1 || o.depth < 1 || o.messages < 1 || o.warmup < 0 || o.warmup >= o.messages)
        throw std::invalid_argument("need --port, and connections, depth, messages >= 1, 0 <= warmup < messages");
    if (o.size < 8) throw std::invalid_argument("--size must be at least 8 (the sequence number)");
    return o;
}

} // namespace

int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);
    Options opt;
    try {
        opt = parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "echo_client: " << e.what() << "\n";
        return 2;
    }

    const auto n = static_cast<std::size_t>(opt.connections);
    std::vector<ConnectionResult> results(n);
    std::latch ready(static_cast<std::ptrdiff_t>(n + 1));
    std::vector<std::thread> threads;
    threads.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        threads.emplace_back(run_connection, std::cref(opt), std::ref(ready), std::ref(results[i]));
    ready.arrive_and_wait();
    const auto start = Clock::now();
    for (auto& t : threads) t.join();

    std::vector<std::int64_t> all;
    Clock::time_point end = start;
    for (auto& r : results) {
        if (!r.error.empty()) {
            std::cerr << "echo_client: " << r.error << "\n";
            return 1;
        }
        all.insert(all.end(), r.rtt_ns.begin(), r.rtt_ns.end());
        end = std::max(end, r.end);
    }

    // Throughput counts every message, warmup included, over the whole run
    const double elapsed = std::chrono::duration<double>(end - start).count();
    const double total = static_cast<double>(opt.messages) * static_cast<double>(n);
    double mean = 0;
    for (auto v : all) mean += static_cast<double>(v);
    mean = all.empty() ? 0 : mean / static_cast<double>(all.size()) / 1e3;

    std::cout << "{\"label\": \"" << opt.label << "\", \"connections\": " << opt.connections
              << ", \"depth\": " << opt.depth << ", \"size\": " << opt.size
              << ", \"nagle\": " << (opt.nagle ? "true" : "false") << ", \"split\": " << (opt.split ? "true" : "false")
              << ", \"messages\": " << static_cast<long>(total) << ", \"elapsed_s\": " << elapsed
              << ", \"msgs_per_s\": " << total / elapsed
              << ", \"mib_per_s\": " << total * static_cast<double>(opt.size) / elapsed / (1 << 20)
              << ", \"mean_us\": " << mean << ", \"p50_us\": " << percentile_us(all, 0.50)
              << ", \"p90_us\": " << percentile_us(all, 0.90) << ", \"p99_us\": " << percentile_us(all, 0.99)
              << ", \"p999_us\": " << percentile_us(all, 0.999) << ", \"max_us\": " << percentile_us(all, 1.0)
              << "}" << std::endl;
}
