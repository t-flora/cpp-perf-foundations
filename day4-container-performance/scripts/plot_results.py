#!/usr/bin/env python3
"""Day 4: turn Google Benchmark JSON into results.csv and containers.png.

Usage: python3 scripts/plot_results.py results/results.json [--out results]
"""
import argparse
import json
from pathlib import Path


def load_rows(path):
    """TODO: return rows of (family, container, pattern, n, ns_per_item).

    Benchmark names look like lookup/map/random/1024 or build/map/1024.
    ns_per_item = 1e9 / items_per_second. If you ran with repetitions, use the median rows.
    """
    benchmarks = json.loads(Path(path).read_text())["benchmarks"]
    raise NotImplementedError


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("json")
    parser.add_argument("--out", default="results")
    args = parser.parse_args()
    rows = load_rows(args.json)
    # TODO: write results.csv
    # TODO: plot: one panel per family/pattern, one line per container, log-log axes,
    #       a fixed color + marker per container across all panels


if __name__ == "__main__":
    main()
