#!/usr/bin/env python3
"""Keep the template branch's shared files in sync with main.

Two things are generated from main and must not be edited on the template branch:
- README.md: main's README with `<!-- main-only -->` ... `<!-- /main-only -->` blocks
  removed and `<!-- template-only` ... `-->` blocks uncommented.
- dayN/reference-tests/*: copies of main's tests for the interfaces the starter headers
  declare (see REFERENCE_TESTS). Each day's reference-tests/CMakeLists.txt is hand-written.

Usage, from main, with the template branch checked out elsewhere:
    git worktree add ../template-wt template
    python3 scripts/template_sync.py --check ../template-wt   # exit 1 on any difference
    python3 scripts/template_sync.py --write ../template-wt   # update the files there
"""
import argparse
import difflib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# main's tests/ file -> template's reference-tests/ file, per day
REFERENCE_TESTS = {
    "day5-realtime-processor": ["check.hpp", "test_ring_buffer.cpp", "test_parser.cpp"],
    "day6-concurrent-calculator": ["check.hpp", "test_bounded_queue.cpp", "test_thread_pool.cpp"],
    "day7-tcp-echo": ["check.hpp", "test_framing.cpp", "test_socket.cpp", "test_echo_server.cpp"],
}

GENERATED_NOTE = "<!-- Generated from main's README by scripts/template_sync.py: edit it there, on main. -->"


def render_readme(text):
    out = []
    state = None  # None, "main-only", or "template-only"
    for lineno, line in enumerate(text.splitlines(), 1):
        marker = line.strip()
        if state is None:
            if marker == "<!-- main-only -->":
                state = "main-only"
            elif marker == "<!-- template-only":
                state = "template-only"
            elif "main-only" in marker or "template-only" in marker:
                sys.exit(f"README.md:{lineno}: unrecognized marker: {marker}")
            else:
                out.append(line)
        elif state == "main-only":
            if marker == "<!-- /main-only -->":
                state = None
        elif marker == "-->":
            state = None
        else:
            out.append(line)
    if state is not None:
        sys.exit(f"README.md: unterminated {state} block")
    out.insert(1, GENERATED_NOTE)
    # Removed blocks leave runs of blank lines behind
    return re.sub(r"\n{3,}", "\n\n", "\n".join(out) + "\n")


def expected_files():
    """Yield (path relative to the repo root, expected content) for every generated file."""
    yield "README.md", render_readme((ROOT / "README.md").read_text())
    for day, names in REFERENCE_TESTS.items():
        for name in names:
            yield f"{day}/reference-tests/{name}", (ROOT / day / "tests" / name).read_text()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="report differences, exit 1 if any")
    mode.add_argument("--write", action="store_true", help="write the generated files")
    parser.add_argument("template_dir", type=Path, help="a checkout of the template branch")
    args = parser.parse_args()

    stale = []
    for rel, want in expected_files():
        path = args.template_dir / rel
        have = path.read_text() if path.exists() else None
        if have == want:
            continue
        stale.append(rel)
        if args.write:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(want)
            print(f"wrote {rel}")
        else:
            diff = difflib.unified_diff((have or "").splitlines(), want.splitlines(),
                                        f"template/{rel}", f"generated/{rel}", lineterm="")
            print("\n".join(list(diff)[:60]))

    if args.check and stale:
        print(f"\n{len(stale)} file(s) out of sync with main: {', '.join(stale)}")
        print("Fix: python3 scripts/template_sync.py --write <template checkout>, then commit on template.")
        return 1
    if not stale:
        print("template is in sync with main")
    return 0


if __name__ == "__main__":
    sys.exit(main())
