#!/usr/bin/env bash

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EPL="$ROOT/build/epl"
CASES="$ROOT/tests/cases"
FILTER="${1:-}"

if [[ ! -x "$EPL" ]]; then
    echo "error: $EPL not found or not executable — run 'make' in build/ first" >&2
    exit 2
fi

pass=0; fail=0; xfail=0; xpass=0
failed_names=()

for src in "$CASES"/*.EP; do
    [[ -e "$src" ]] || continue
    name="$(basename "$src" .EP)"
    [[ -n "$FILTER" && "$name" != *"$FILTER"* ]] && continue

    expected_file="${src%.EP}.expected"
    if [[ ! -f "$expected_file" ]]; then
        echo "SKIP  $name (no .expected file)"
        continue
    fi

    actual="$("$EPL" "$src" 2>/dev/null)"
    expected="$(cat "$expected_file")"

    if [[ "$actual" == "$expected" ]]; then
        if [[ "$name" == xfail_* ]]; then
            echo "XPASS $name  (expected failure now passes — promote it)"
            ((xpass++))
        else
            echo "PASS  $name"
            ((pass++))
        fi
    else
        if [[ "$name" == xfail_* ]]; then
            echo "XFAIL $name  (known broken)"
            ((xfail++))
        else
            echo "FAIL  $name"
            diff <(echo "$expected") <(echo "$actual") \
                | sed 's/^/        /' | head -20
            ((fail++))
            failed_names+=("$name")
        fi
    fi
done

echo
echo "───────────────────────────────────────"
echo "pass $pass   fail $fail   xfail $xfail   xpass $xpass"

if (( fail > 0 )); then
    echo "failed: ${failed_names[*]}"
    exit 1
fi
exit 0
