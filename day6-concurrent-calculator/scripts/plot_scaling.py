#!/usr/bin/env python3
"""Plot bench_scaling results: speedup vs. threads, and counter throughput vs. threads.

Usage:
    python3 scripts/plot_scaling.py results/scaling.json [--out results/scaling.png]

Needs matplotlib, e.g.: uv run --with matplotlib scripts/plot_scaling.py results/scaling.json
With --benchmark_repetitions, the median of the repetitions is used.
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

SURFACE = "#fcfcfb"
TEXT = "#2b2b2a"
MUTED = "#6b6b68"
GRID = "#e4e3df"

# Fixed color + marker per series; markers keep lines distinguishable without color
SPEEDUP_STYLE = {
    "primes/dynamic": ("#2a78d6", "o", "primes, dynamic chunks"),
    "primes/static": ("#eb6834", "s", "primes, static slices"),
    "sum/parallel": ("#1baf7a", "^", "sum 256 MB (memory-bound)"),
}
COUNTER_STYLE = {
    "per_thread_padded128": ("#2a78d6", "o", "per-thread, padded 128 B"),
    "per_thread_padded64": ("#eb6834", "s", "per-thread, padded 64 B"),
    "per_thread_adjacent": ("#1baf7a", "^", "per-thread, same line"),
    "shared_atomic": ("#eda100", "D", "one shared atomic"),
    "mutex": ("#e87ba4", "v", "mutex"),
}


def load(path):
    """Return {run_name: benchmark row}, using medians when repetitions were run."""
    rows = json.loads(Path(path).read_text())["benchmarks"]
    has_aggregates = any(r.get("run_type") == "aggregate" for r in rows)
    keep = {}
    for r in rows:
        if has_aggregates and r.get("aggregate_name") != "median":
            continue
        if not has_aggregates and r.get("run_type") != "iteration":
            continue
        keep[r["run_name"]] = r
    return keep


def style_axes(ax, title, xlabel, ylabel):
    ax.set_facecolor(SURFACE)
    ax.set_title(title, loc="left", fontsize=10, color=TEXT)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.tick_params(length=0)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("json")
    parser.add_argument("--out", default="results/scaling.png")
    args = parser.parse_args()
    runs = load(args.json)

    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 9, "text.color": TEXT, "axes.labelcolor": MUTED,
                         "axes.edgecolor": GRID, "xtick.color": MUTED, "ytick.color": MUTED})
    fig, (left, right) = plt.subplots(1, 2, figsize=(12, 4.8), facecolor=SURFACE)

    # Speedup vs. the serial version of each workload
    baselines = {"primes": runs["primes/serial/real_time"]["real_time"],
                 "sum": runs["sum/serial/real_time"]["real_time"]}
    series = defaultdict(list)
    for name, r in runs.items():
        m = re.fullmatch(r"(primes/static|primes/dynamic|sum/parallel)/(\d+)/real_time", name)
        if m:
            family, threads = m.group(1), int(m.group(2))
            series[family].append((threads, baselines[family.split("/")[0]] / r["real_time"]))
    max_threads = max(t for pts in series.values() for t, _ in pts)
    left.plot([1, max_threads], [1, max_threads], color=MUTED, linestyle="--", linewidth=1, label="ideal")
    for family, (color, marker, label) in SPEEDUP_STYLE.items():
        pts = sorted(series[family])
        left.plot([t for t, _ in pts], [s for _, s in pts], color=color, marker=marker, markersize=6,
                  linewidth=2, markeredgecolor=SURFACE, label=label)
    left.axvline(4.5, color=GRID, linewidth=6, zorder=0)
    left.text(4.6, max_threads * 0.95, "efficiency cores →", color=MUTED, fontsize=8, va="top")
    style_axes(left, "Speedup over serial (higher is better)", "threads", "speedup")
    left.set_xticks(range(1, max_threads + 1))
    left.legend(frameon=False, loc="upper left")

    # Total counter throughput vs. threads
    counters = defaultdict(list)
    for name, r in runs.items():
        m = re.fullmatch(r"counter/(\w+)/real_time/threads:(\d+)", name)
        if m:
            counters[m.group(1)].append((int(m.group(2)), r["items_per_second"] / 1e9))
    for kind, (color, marker, label) in COUNTER_STYLE.items():
        pts = sorted(counters[kind])
        right.plot([t for t, _ in pts], [v for _, v in pts], color=color, marker=marker, markersize=6,
                   linewidth=2, markeredgecolor=SURFACE, label=label)
    right.set_yscale("log")
    style_axes(right, "Counter increments, all threads combined (higher is better)", "threads",
               "billion increments / s (log)")
    right.set_xticks(range(1, max_threads + 1))
    right.legend(frameon=False, loc="center left", bbox_to_anchor=(1.0, 0.5))

    fig.tight_layout()
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.out, dpi=150)
    print(f"Wrote {args.out}")


if __name__ == "__main__":
    main()
