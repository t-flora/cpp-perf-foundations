// processor: reads a CSV trade stream, keeps per-symbol sliding-window statistics, writes
// event-time snapshots, and reports throughput plus the latency distribution per record.
//
//   processor [--pipeline fast|naive] [--window-ms N] [--interval-ms N] [--capacity N]
//             [--output FILE|-|none] [input.csv|-]
//
// Input defaults to stdin, so it works on a live stream:
//   python3 scripts/generate_trades.py --trades 1000000 | ./build/processor --output none
#include <stream/histogram.hpp>
#include <stream/line_reader.hpp>
#include <stream/pipeline.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    std::string pipeline = "fast";
    std::string input = "-";
    std::string output = "-";
    stream::Config config;
};

[[noreturn]] void usage(const char* msg) {
    std::fprintf(stderr,
                 "error: %s\n"
                 "usage: processor [--pipeline fast|naive] [--window-ms N] [--interval-ms N]\n"
                 "                 [--capacity N] [--output FILE|-|none] [input.csv|-]\n",
                 msg);
    std::exit(2);
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) usage(("missing value for " + arg).c_str());
            return argv[++i];
        };
        auto positive = [&](const std::string& v) {
            const long long n = std::atoll(v.c_str());
            if (n <= 0) usage(("expected a positive number for " + arg).c_str());
            return n;
        };
        if (arg == "--pipeline") {
            o.pipeline = value();
            if (o.pipeline != "fast" && o.pipeline != "naive") usage("--pipeline must be fast or naive");
        } else if (arg == "--window-ms") {
            o.config.window_ns = positive(value()) * 1'000'000;
        } else if (arg == "--interval-ms") {
            o.config.interval_ns = positive(value()) * 1'000'000;
        } else if (arg == "--capacity") {
            o.config.ring_capacity = static_cast<std::size_t>(positive(value()));
        } else if (arg == "--output") {
            o.output = value();
        } else if (!arg.empty() && arg[0] == '-' && arg != "-") {
            usage(("unknown option " + arg).c_str());
        } else {
            o.input = arg;
        }
    }
    return o;
}

// Cost of the measurement itself: two clock reads with nothing between them
stream::LatencyHistogram clock_overhead() {
    stream::LatencyHistogram h;
    for (int i = 0; i < 1'000'000; ++i) {
        const auto t0 = Clock::now();
        const auto t1 = Clock::now();
        h.record(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
    }
    return h;
}

void print_latency(const char* label, const stream::LatencyHistogram& h) {
    std::fprintf(stderr, "%-15s p50 %llu ns | p90 %llu | p99 %llu | p99.9 %llu | p99.99 %llu | max %llu | mean %.1f\n", label,
                 static_cast<unsigned long long>(h.percentile(50)), static_cast<unsigned long long>(h.percentile(90)),
                 static_cast<unsigned long long>(h.percentile(99)), static_cast<unsigned long long>(h.percentile(99.9)),
                 static_cast<unsigned long long>(h.percentile(99.99)), static_cast<unsigned long long>(h.max()), h.mean());
}

void print_summary(const Options& o, const stream::Counters& c, std::size_t growths, double seconds,
                   const stream::LatencyHistogram& latency, const stream::LatencyHistogram& overhead) {
    std::fprintf(stderr, "pipeline        %s\n", o.pipeline.c_str());
    std::fprintf(stderr, "lines           %llu\n", static_cast<unsigned long long>(c.lines));
    std::fprintf(stderr, "accepted        %llu\n", static_cast<unsigned long long>(c.accepted));
    std::fprintf(stderr, "rejected        %llu", static_cast<unsigned long long>(c.total_rejected()));
    for (std::size_t i = 0; i < c.rejected.size(); ++i) {
        if (c.rejected[i]) std::fprintf(stderr, " %s=%llu", stream::kRejectNames[i], static_cast<unsigned long long>(c.rejected[i]));
    }
    std::fprintf(stderr, "\nsnapshot rows   %llu\n", static_cast<unsigned long long>(c.snapshot_rows));
    if (o.pipeline == "fast") std::fprintf(stderr, "ring growths    %zu\n", growths);
    std::fprintf(stderr, "elapsed         %.3f s (%.2fM lines/s, including I/O)\n", seconds,
                 static_cast<double>(c.lines) / seconds / 1e6);
    print_latency("latency/line", latency);
    print_latency("clock overhead", overhead);
}

template <typename Pipeline, typename Line>
void timed_process(Pipeline& p, const Line& line, stream::LatencyHistogram& latency) {
    const auto t0 = Clock::now();
    p.process(line);
    const auto t1 = Clock::now();
    latency.record(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
}

} // namespace

int main(int argc, char** argv) {
    const Options o = parse_args(argc, argv);
    const bool to_stdout = o.output == "-";
    const bool no_output = o.output == "none";

    const auto overhead = clock_overhead();
    stream::LatencyHistogram latency;
    const auto start = Clock::now();

    if (o.pipeline == "naive") {
        std::ifstream in_file;
        std::istream* in = &std::cin;
        if (o.input != "-") {
            in_file.open(o.input);
            if (!in_file) usage(("cannot open " + o.input).c_str());
            in = &in_file;
        }
        std::ofstream out_file;
        std::ostream* out = to_stdout ? &std::cout : nullptr;
        if (!to_stdout && !no_output) {
            out_file.open(o.output);
            out = &out_file;
        }

        stream::NaivePipeline p(o.config, out);
        std::string line;
        while (std::getline(*in, line)) timed_process(p, line, latency);
        p.finish();
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        print_summary(o, p.counters(), 0, seconds, latency, overhead);
    } else {
        std::FILE* in = o.input == "-" ? stdin : std::fopen(o.input.c_str(), "rb");
        if (!in) usage(("cannot open " + o.input).c_str());
        std::FILE* out_file = to_stdout ? stdout : (no_output ? nullptr : std::fopen(o.output.c_str(), "wb"));

        stream::Counters counters;
        std::size_t growths = 0;
        {
            stream::OutputBuffer out(out_file);
            stream::FastPipeline p(o.config, no_output ? nullptr : &out);
            stream::LineReader reader(in);
            std::string_view line;
            while (reader.next(line)) timed_process(p, line, latency);
            p.finish();
            counters = p.counters();
            growths = p.ring_growths();
        } // flushes the output buffer
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        print_summary(o, counters, growths, seconds, latency, overhead);
        if (in != stdin) std::fclose(in);
        if (out_file && out_file != stdout) std::fclose(out_file);
    }
    return 0;
}
