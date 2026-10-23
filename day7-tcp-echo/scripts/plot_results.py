#!/usr/bin/env python3
"""Plot run_experiments.py results: latency vs. message size, pipelining, and concurrency.

Usage:
    python3 scripts/plot_results.py results/macos.json results/linux-docker.json [--out results/echo.png]

Needs matplotlib, e.g.: uv run --with matplotlib scripts/plot_results.py results/*.json
Each point is the median of the repetitions.
"""
import argparse
import json
import statistics
from collections import defaultdict
from pathlib import Path

SURFACE = "#fcfcfb"
TEXT = "#2b2b2a"
MUTED = "#6b6b68"
GRID = "#e4e3df"

# Fixed color + marker per platform; the server design is the line style
PLATFORM_STYLE = [("#2a78d6", "o"), ("#eb6834", "s")]
SERVER_STYLE = {"threads": ("-", "thread per connection"), "poll": ("--", "poll() loop")}


def load(path):
    """Return (label, {(experiment, server, x): median row}) for one results file."""
    data = json.loads(Path(path).read_text())
    groups = defaultdict(list)
    for r in data["runs"]:
        x = {"size": r["size"], "depth": r["depth"], "connections": r["connections"]}.get(r["experiment"])
        if x is not None:
            groups[(r["experiment"], r["server"], x)].append(r)
    medians = {k: {m: statistics.median(r[m] for r in rs) for m in ("msgs_per_s", "p50_us", "p99_us")}
               for k, rs in groups.items()}
    label = "macOS (native)" if data["meta"]["system"].startswith("macOS") else "Linux (Docker VM)"
    return label, medians


def series(medians, experiment, server, metric):
    pts = sorted((x, row[metric]) for (e, s, x), row in medians.items() if e == experiment and s == server)
    return [x for x, _ in pts], [y for _, y in pts]


def style_axes(ax, title, xlabel, ylabel):
    ax.set_facecolor(SURFACE)
    ax.set_title(title, loc="left", fontsize=10, color=TEXT)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.grid(True, which="major", color=GRID, linewidth=0.8)
    ax.tick_params(length=0)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("json", nargs="+")
    parser.add_argument("--out", default="results/echo.png")
    args = parser.parse_args()
    platforms = [load(p) for p in args.json]

    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 9, "text.color": TEXT, "axes.labelcolor": MUTED,
                         "axes.edgecolor": GRID, "xtick.color": MUTED, "ytick.color": MUTED})
    fig, axes = plt.subplots(2, 2, figsize=(12, 8.5), facecolor=SURFACE)
    (size_ax, conn_ax), (depth_tput_ax, depth_lat_ax) = axes
    line = {"linewidth": 2, "markersize": 6, "markeredgecolor": SURFACE}

    for (label, med), (color, marker) in zip(platforms, PLATFORM_STYLE):
        x, y = series(med, "size", "threads", "p50_us")
        size_ax.plot(x, y, color=color, marker=marker, label=label, **line)

        for server, (ls, server_label) in SERVER_STYLE.items():
            x, y = series(med, "connections", server, "msgs_per_s")
            conn_ax.plot(x, y, color=color, marker=marker, linestyle=ls, label=f"{label}, {server_label}", **line)

        x, tput = series(med, "depth", "threads", "msgs_per_s")
        depth_tput_ax.plot(x, tput, color=color, marker=marker, label=label, **line)
        _, p50 = series(med, "depth", "threads", "p50_us")
        depth_lat_ax.plot(x, p50, color=color, marker=marker, label=f"{label}, measured p50", **line)
        # Little's law: requests in flight = throughput × latency
        depth_lat_ax.plot(x, [d / t * 1e6 for d, t in zip(x, tput)], color=color, linestyle=":", linewidth=1.5,
                          label=f"{label}, depth / throughput")

    style_axes(size_ax, "Round-trip time vs. message size, 1 connection (lower is better)", "payload bytes",
               "p50 round trip (µs, log)")
    size_ax.legend(frameon=False, loc="upper left")
    style_axes(conn_ax, "Total throughput vs. connections, 64 B (higher is better)", "connections",
               "round trips / s (log)")
    conn_ax.legend(frameon=False, loc="upper left", fontsize=8)
    style_axes(depth_tput_ax, "Pipelining: throughput vs. requests in flight, 1 connection", "requests in flight",
               "round trips / s (log)")
    depth_tput_ax.legend(frameon=False, loc="upper left")
    style_axes(depth_lat_ax, "Pipelining: latency vs. requests in flight", "requests in flight",
               "round trip (µs, log)")
    depth_lat_ax.legend(frameon=False, loc="upper left", fontsize=8)

    fig.tight_layout()
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.out, dpi=150)
    print(f"Wrote {args.out}")


if __name__ == "__main__":
    main()
