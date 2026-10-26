#!/usr/bin/env bash
# Check a checkout of the template branch against main:
#   1. its README and reference tests match what main generates (scripts/template_sync.py)
#   2. every starter header compiles on its own, warning-free
#   3. every day configures and builds, and the reference tests compile against the
#      starter headers (they aren't run: against stubs they're meant to fail)
# Usage, from main: git worktree add ../template-wt template && scripts/check_template.sh ../template-wt
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
tpl="$(cd "${1:?usage: $0 <template checkout>}" && pwd)"
cxx="${CXX:-c++}"
warnings=(-Wall -Wextra -Wpedantic -Werror)

echo "==> README and reference tests in sync with main"
python3 "$root/scripts/template_sync.py" --check "$tpl"

echo "==> Starter headers compile on their own"
cd "$tpl"
headers=0
for header in day*/include/*/*.hpp; do
    day="${header%%/*}"
    # As a translation unit that only includes it, so it must include what it uses
    echo "#include \"$tpl/$header\"" | "$cxx" -std=c++20 "${warnings[@]}" -I"$tpl/$day/include" -fsyntax-only -x c++ -
    headers=$((headers + 1))
done
echo "$headers headers OK"

echo "==> Every day configures and builds, with reference tests where they exist"
for dir in day*/; do
    day="${dir%/}"
    num="${day#day}"
    num="${num%%-*}"
    args=()
    [ -d "$day/reference-tests" ] && args+=(-DDAY"${num}"_REFERENCE_TESTS=ON)
    [ "$num" = 7 ] && args+=(-DDAY7_REFERENCE_POLL=ON)
    cmake -S "$day" -B "build/check/$day" -DCMAKE_CXX_FLAGS="${warnings[*]}" ${args[@]+"${args[@]}"} > /dev/null
    cmake --build "build/check/$day" --parallel > /dev/null
    echo "$day OK"
done

echo "Template checks passed"
