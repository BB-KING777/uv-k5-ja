#!/usr/bin/env bash
# Compiles App/app/si4732.c for the host and replays key events against it.
# Nothing here ships on the radio; it exists so key handling can be checked
# without flashing.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="${1:-/tmp/ja-k5-keytest}"

mkdir -p "$OUT"
gcc -std=gnu11 -Wall -Wno-int-to-pointer-cast -DENABLE_SI4732 -DENABLE_FEAT_F4HWN \
    -I"$ROOT/App" -I"$HERE/fake" \
    "$HERE/keytest.c" "$ROOT/App/app/si4732.c" \
    -o "$OUT/keytest"
"$OUT/keytest"
