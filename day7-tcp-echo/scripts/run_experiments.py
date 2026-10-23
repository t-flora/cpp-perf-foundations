#!/usr/bin/env python3
"""Run every Day 7 experiment against a fresh echo_server and save the results.

    scripts/run_experiments.py [--build build] [--out results/<os>.json] [--reps 3]

Each configuration runs --reps times; plot_results.py takes the median. Standard library only.
"""
import argparse
import json
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

SIZES = [64, 512, 4096, 32768, 262144, 1048576]
DEPTHS = [1, 2, 4, 8, 16, 32, 64, 128]
CONNECTIONS = [1, 2, 4, 8, 16, 32, 64, 128]


def experiments():
    """(experiment, server mode, client args) for every configuration."""
    for size in SIZES:
        # Fewer round trips for big messages, so each run takes about a second
        messages = max(200, min(10000, 10000 * 4096 // size))
        yield "size", "threads", ["--size", size, "--messages", messages, "--warmup", messages // 10]
    for nagle in (False, True):
        for split in (False, True):
            # Nagle + split can stall ~40 ms per round trip (Linux), so keep that one short
            messages, warmup = (60, 10) if nagle and split else (5000, 500)
            yield "nagle", "threads", ["--messages", messages, "--warmup", warmup] + \
                (["--nagle"] if nagle else []) + (["--split"] if split else [])
    for depth in DEPTHS:
        yield "depth", "threads", ["--depth", depth, "--messages", 20000, "--warmup", 2000]
    for mode in ("threads", "poll"):
        for c in CONNECTIONS:
            messages = max(400, 40000 // c)
            yield "connections", mode, ["--connections", c, "--messages", messages, "--warmup", messages // 10]


def start_server(build, mode):
    proc = subprocess.Popen([str(build / "echo_server"), "--port", "0", "--mode", mode],
                            stdout=subprocess.PIPE, text=True)
    assert proc.stdout is not None
    match = re.search(r":(\d+)$", proc.stdout.readline().strip())
    if not match:
        proc.kill()
        sys.exit("echo_server did not report a port")
    return proc, int(match.group(1))


def main():
    parser = argparse.ArgumentParser(description="Run the Day 7 experiments")
    parser.add_argument("--build", default="build", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--reps", type=int, default=3)
    args = parser.parse_args()
    system = platform.system().lower()
    out = args.out or Path("results") / f"{'macos' if system == 'darwin' else system}.json"

    # Transport costs first, with no server running
    probe = subprocess.run([str(args.build / "transport_probe")], check=True, capture_output=True, text=True)
    probes = [json.loads(line) for line in probe.stdout.splitlines()]

    runs = []
    servers = {}
    try:
        for mode in ("threads", "poll"):
            servers[mode] = start_server(args.build, mode)
        configs = list(experiments())
        for i, (name, mode, client_args) in enumerate(configs, 1):
            argv = [str(args.build / "echo_client"), "--port", str(servers[mode][1])] + [str(a) for a in client_args]
            print(f"[{i}/{len(configs)}] {name} {mode} {' '.join(map(str, client_args))}", file=sys.stderr)
            for rep in range(args.reps):
                result = json.loads(subprocess.run(argv, check=True, capture_output=True, text=True).stdout)
                runs.append({"experiment": name, "server": mode, "rep": rep, **result})
    finally:
        for proc, _ in servers.values():
            proc.terminate()
            proc.wait()

    out.parent.mkdir(parents=True, exist_ok=True)
    meta = {"system": platform.platform(), "machine": platform.machine(), "cpus": os.cpu_count(),
            "reps": args.reps}
    out.write_text(json.dumps({"meta": meta, "probes": probes, "runs": runs}, indent=1) + "\n")
    print(f"wrote {len(runs)} runs to {out}", file=sys.stderr)


if __name__ == "__main__":
    main()
