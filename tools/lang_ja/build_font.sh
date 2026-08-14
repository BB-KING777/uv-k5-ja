#!/usr/bin/env bash
# Regenerates App/font_ja.c from the Misaki Gothic BDF font, embedding only the
# characters used by the firmware sources.
#
# Usage: tools/lang_ja/build_font.sh [path/to/misaki_gothic.bdf]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BDF="${1:-$ROOT/tools/lang_ja/misaki_gothic.bdf}"

if [[ ! -f "$BDF" ]]; then
  echo "Misaki BDF not found at $BDF" >&2
  echo "Download misaki_bdf_*.zip from https://littlelimit.net/misaki.htm" >&2
  exit 1
fi

mapfile -t SOURCES < <(find "$ROOT/App" -name '*.c' -o -name '*.h' -o -name '*.inc')

python3 "$ROOT/tools/lang_ja/misaki2c.py" \
  --bdf "$BDF" \
  --scan "${SOURCES[@]}" \
  --out-c "$ROOT/App/font_ja.c" \
  --out-h "$ROOT/App/font_ja.h"
