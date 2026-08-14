#!/usr/bin/env bash
# Renders sample screens with the firmware's own text routines and converts
# them to PNG. Purely a development aid, nothing here ships on the radio.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${1:-/tmp/uvk5-ja-sim}"
mkdir -p "$OUT"
cd "$OUT"
gcc -std=gnu11 -DENABLE_LANG_JA -DENABLE_SMALL_BOLD -DENABLE_FEAT_F4HWN \
    -I"$ROOT/App" \
    "$ROOT/tools/lang_ja/sim/render.c" \
    "$ROOT/App/ui/helper.c" \
    "$ROOT/App/font.c" \
    "$ROOT/App/font_ja.c" \
    "$ROOT/App/font_ja_util.c" \
    -o render
./render
for f in *.pgm; do python3 -c "
import sys,zlib,struct
p=sys.argv[1]
d=open(p,'rb').read().split()
w,h=int(d[1]),int(d[2]); px=[int(v) for v in d[4:4+w*h]]
raw=b''.join(b'\x00'+bytes(px[y*w:(y+1)*w]) for y in range(h))
def chunk(t,b): 
    c=t+b; return struct.pack('>I',len(b))+c+struct.pack('>I',zlib.crc32(c))
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,0,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')
open(p[:-4]+'.png','wb').write(png)
" "$f"; done
echo "$OUT"
