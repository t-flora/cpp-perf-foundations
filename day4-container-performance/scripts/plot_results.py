#!/usr/bin/env python3
"""Turn Google Benchmark JSON from bench_containers into a tidy CSV and one PNG of plots.

Usage:
    python3 scripts/plot_results.py results/results.json [--out results]

Writes <out>/results.csv (one row per family/container/pattern/N) and <out>/containers.png.
With --benchmark_repetitions, the median of the repetitions is used.
Needs matplotlib, e.g.: uv run --with matplotlib scripts/plot_results.py results/results.json
"""
import argparse
import csv
import json
from pathlib import Path

# One fixed color + marker per container, the same in every panel. Markers keep the series
# distinguishable without color
STYLE = {
    "map":                    ("#2a78d6", "o"),
    "unordered_map":          ("#eb6834", "s"),
    "sorted_vector":          ("#1baf7a", "^"),
    "vector":                 ("#eda100", "D"),
    "unordered_map_reserved": ("#e87ba4", "v"),
    "list":                   ("#008300", "P"),
}

PANELS = [
    ("lookup", "random", "Lookup, random keys"),
    ("lookup", "sequential", "Lookup, keys in sorted order"),
    ("build", "", "Build from n entries (+ destroy)"),
    ("iterate", "", "Iterate, sum all values"),
    ("insert_sorted", "", "Sorted insertion, linear search"),
]

TEXT = "#2b2b2a"
MUTED = "#6b6b68"
GRID = "#e4e3df"


def load_rows(path):
    """Return tidy rows: family, container, pattern, n, ns_per_item."""
    benchmarks = json.loads(Path(path).read_text())["benchmarks"]
    has_aggregates = any(b.get("run_type") == "aggregate" for b in benchmarks)
    rows = []
    for b in benchmarks:
        if has_aggregates:
            if b.get("aggregate_name") != "median":
                continue
        elif b.get("run_type") != "iteration":
            continue
        if "items_per_second" not in b:
            continue  # BigO / RMS rows
        parts = b["run_name"].split("/")
        family, container, n = parts[0], parts[1], int(parts[-1])
        pattern = parts[2] if len(parts) == 4 else ""
        rows.append({
            "family": family,
            "container": container,
            "pattern": pattern,
            "n": n,
            "ns_per_item": 1e9 / b["items_per_second"],
        })
    rows.sort(key=lambda r: (r["family"], r["pattern"], r["container"], r["n"]))
    return rows


def write_csv(rows, path):
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["family", "container", "pattern", "n", "ns_per_item"])
        writer.writeheader()
        for r in rows:
            writer.writerow({**r, "ns_per_item": f"{r['ns_per_item']:.3f}"})


def plot(rows, path):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import LogLocator

    plt.rcParams.update({
        "font.size": 9,
        "text.color": TEXT,
        "axes.labelcolor": MUTED,
        "axes.edgecolor": GRID,
        "xtick.color": MUTED,
        "ytick.color": MUTED,
    })
    fig, axes = plt.subplots(2, 3, figsize=(13, 7.5), facecolor="#fcfcfb")
    flat = axes.flatten()

    for ax, (family, pattern, title) in zip(flat, PANELS):
        ax.set_facecolor("#fcfcfb")
        panel = [r for r in rows if r["family"] == family and r["pattern"] == pattern]
        for container, (color, marker) in STYLE.items():
            series = [r for r in panel if r["container"] == container]
            if not series:
                continue
            ax.plot([r["n"] for r in series], [r["ns_per_item"] for r in series],
                    color=color, marker=marker, markersize=6, linewidth=2,
                    markeredgecolor="#fcfcfb", markeredgewidth=1, label=container)
        ax.set_xscale("log", base=2)
        ax.set_yscale("log")
        ax.yaxis.set_major_locator(LogLocator(base=10))
        ax.set_title(title, loc="left", fontsize=10, color=TEXT)
        ax.set_xlabel("n (elements)")
        ax.set_ylabel("ns per item")
        ax.grid(True, which="major", color=GRID, linewidth=0.8)
        ax.tick_params(length=0)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)

    # The sixth cell holds the shared legend
    legend_ax = flat[len(PANELS)]
    legend_ax.axis("off")
    handles, labels = [], []
    for ax in flat[: len(PANELS)]:
        for h, label in zip(*ax.get_legend_handles_labels()):
            if label not in labels:
                handles.append(h)
                labels.append(label)
    legend_ax.legend(handles, labels, loc="center left", frameon=False, fontsize=10,
                     title="Container", title_fontsize=10)

    fig.suptitle("STL container cost per item vs. size (lower is better, log-log)",
                 x=0.01, ha="left", fontsize=12, color=TEXT)
    fig.tight_layout()
    fig.savefig(path, dpi=150)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("json", help="Google Benchmark JSON output (--benchmark_out)")
    parser.add_argument("--out", default="results", help="output directory (default: results)")
    args = parser.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    rows = load_rows(args.json)
    write_csv(rows, out / "results.csv")
    plot(rows, out / "containers.png")
    print(f"Wrote {out / 'results.csv'} ({len(rows)} rows) and {out / 'containers.png'}")


if __name__ == "__main__":
    main()
