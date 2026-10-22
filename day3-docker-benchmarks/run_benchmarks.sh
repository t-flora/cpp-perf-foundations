#!/usr/bin/env bash
# Day 3: build the image, run it, and write results/<timestamp>.json.
# Extra arguments should go to the benchmark binary.
set -euo pipefail
cd "$(dirname "$0")"

# TODO: docker build -t <image> .
# TODO: docker run --rm, mounting ./results into the container, with
#       --benchmark_out=/results/<timestamp>.json --benchmark_out_format=json "$@"
echo "TODO: implement run_benchmarks.sh" >&2
exit 1
