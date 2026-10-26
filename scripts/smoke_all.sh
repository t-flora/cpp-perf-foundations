#!/usr/bin/env bash
# Run every benchmark and program briefly, to catch ones that build but crash or hang.
# Timings here mean nothing. Run scripts/test_all.sh first: this uses its build/<day>/ trees.
# Usage: scripts/smoke_all.sh
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
b=build

# Output is shown only if the command fails
run() {
    echo "==> $*"
    local out
    if ! out="$("$@" 2>&1)"; then
        echo "$out"
        echo "FAILED: $*"
        return 1
    fi
}

run "$b/day2-header-library/benchmarks/simple_benchmark"

# --benchmark_dry_run: one iteration of every benchmark, no repetitions
for bench in day3-docker-benchmarks/bench_sorting day4-container-performance/bench_containers \
    day5-realtime-processor/bench_components day6-concurrent-calculator/bench_scaling; do
    run "$b/$bench" --benchmark_dry_run=true
done

for pipeline in fast naive; do
    run "$b/day5-realtime-processor/processor" --pipeline "$pipeline" --output none \
        day5-realtime-processor/data/sample_trades.csv
done
run "$b/day6-concurrent-calculator/race_demo"

# Day 7: a server per mode, a short client run against each, then the transport probe
day7="$b/day7-tcp-echo"
for mode in threads poll; do
    log="$(mktemp)"
    "$day7/echo_server" --mode "$mode" > "$log" &
    server=$!
    port=""
    for _ in $(seq 50); do
        port="$(sed -n 's/^listening on .*:\([0-9]*\)$/\1/p' "$log")"
        [ -n "$port" ] && break
        sleep 0.1
    done
    [ -n "$port" ] || { echo "echo_server ($mode) didn't start"; cat "$log"; exit 1; }
    run "$day7/echo_client" --port "$port" --connections 4 --depth 4 --messages 500 --warmup 50
    run "$day7/echo_client" --port "$port" --nagle --split --messages 20 --warmup 2
    kill -TERM "$server"
    wait "$server"
    rm -f "$log"
done
run "$day7/transport_probe"

echo "All smoke runs passed"
