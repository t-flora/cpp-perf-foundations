// End to end: both pipelines, fed the committed sample stream the way processor feeds them,
// must produce byte-identical snapshots and identical counters.
#include "check.hpp"

#include <stream/line_reader.hpp>
#include <stream/pipeline.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using test::check;

namespace {

struct Result {
    std::string output;
    stream::Counters counters;
    std::size_t growths = 0;
};

Result run_naive(const stream::Config& cfg) {
    std::ifstream in(SAMPLE_TRADES);
    std::ostringstream out;
    stream::NaivePipeline p(cfg, &out);
    std::string line;
    while (std::getline(in, line)) p.process(line);
    p.finish();
    return {out.str(), p.counters(), 0};
}

Result run_fast(const stream::Config& cfg, std::size_t read_chunk) {
    std::FILE* in = std::fopen(SAMPLE_TRADES, "rb");
    stream::OutputBuffer out; // no file: keeps everything in memory
    stream::FastPipeline p(cfg, &out);
    stream::LineReader reader(in, read_chunk);
    std::string_view line;
    while (reader.next(line)) p.process(line);
    p.finish();
    std::fclose(in);
    return {out.str(), p.counters(), p.ring_growths()};
}

} // namespace

int main() {
    stream::Config cfg;
    cfg.window_ns = 10'000'000'000;   // 10 s, shorter than the ~40 s sample, so eviction happens
    cfg.interval_ns = 1'000'000'000;

    const Result naive = run_naive(cfg);
    const Result fast = run_fast(cfg, 1 << 16);

    check(naive.counters == fast.counters, "counters agree");
    check(naive.output == fast.output, "snapshot output is byte-identical");

    // Sanity-check the sample itself: generate_trades.py --trades 2000 --bad-rate 0.02 --seed 7
    check(fast.counters.accepted == 2000, "2000 valid trades accepted");
    check(fast.counters.lines == fast.counters.accepted + fast.counters.total_rejected(), "every line accounted for");
    for (std::size_t i = 0; i < fast.counters.rejected.size(); ++i) {
        check(fast.counters.rejected[i] > 0, std::string("sample exercises rejection: ") + stream::kRejectNames[i]);
    }
    check(fast.counters.snapshot_rows > 0 && fast.growths == 0, "snapshots emitted, no ring growth at default capacity");
    check(fast.output.rfind(stream::kHeader, 0) == 0, "output starts with the header");

    // A tiny ring capacity forces growth, and a tiny read buffer forces partial lines and
    // buffer growth in LineReader. Results must not change
    stream::Config tiny = cfg;
    tiny.ring_capacity = 2;
    const Result stressed = run_fast(tiny, 8);
    check(stressed.growths > 0, "tiny capacity makes ring buffers grow");
    check(stressed.output == naive.output && stressed.counters == naive.counters,
          "growth and partial reads don't change results");

    return test::finish("pipelines");
}
