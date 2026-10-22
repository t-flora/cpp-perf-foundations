#!/usr/bin/env bash
# Build the benchmark image, run it, and write results/docker-<timestamp>.json.
# Extra arguments go to the benchmark binary and override the defaults below, e.g.
#   ./run_benchmarks.sh --benchmark_filter=radix --benchmark_repetitions=3
set -euo pipefail
cd "$(dirname "$0")"

IMAGE=${IMAGE:-day3-sorting-bench}
UBUNTU_VERSION=${UBUNTU_VERSION:-24.04} # base image for both Dockerfile stages
OUT="docker-$(date +%Y%m%d-%H%M%S).json"
mkdir -p results

docker build --build-arg UBUNTU_VERSION="$UBUNTU_VERSION" -t "$IMAGE" .

# --user: on Linux, files written to the bind mount would otherwise be owned by root
docker run --rm \
    --user "$(id -u):$(id -g)" \
    -v "$PWD/results:/results" \
    "$IMAGE" \
    --benchmark_repetitions=10 \
    --benchmark_min_time=0.1s \
    --benchmark_display_aggregates_only=true \
    --benchmark_out="/results/$OUT" \
    --benchmark_out_format=json \
    "$@"

echo "Results written to results/$OUT"
