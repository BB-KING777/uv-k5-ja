#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
probe.py — 最小限の疎通確認。dump_regs.py が動かないときの切り分け用。

ポートを一番素朴な方法で開き、ハンドシェイク(0x0514)を1回だけ投げて、
返ってきた生バイトをそのまま表示します。解釈は一切しません。

    python probe.py COM19
"""
import sys, time, serial

OBF = bytes([0x16,0x6C,0x14,0xE6,0x2E,0x91,0x0D,0x40,
             0x21,0x35,0xD5,0x40,0x13,0x03,0xE9,0x80])

def crc16(d):
    c = 0
    for b in d:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c

def frame(payload):
    body = bytearray(payload) + crc16(payload).to_bytes(2, "little")
    for i in range(len(body)):
        body[i] ^= OBF[i % 16]
    return b"\xAB\xCD" + len(payload).to_bytes(2, "little") + bytes(body) + b"\xDC\xBA"

port = sys.argv[1] if len(sys.argv) > 1 else sys.exit(__doc__)

print(f"{port} を開きます ...", end=" ", flush=True)
s = serial.Serial(port, 38400, timeout=1)
print("OK")
time.sleep(0.5)

cmd = frame((0x0514).to_bytes(2,"little") + (4).to_bytes(2,"little") + b"\x78\x56\x34\x12")
print("送信:", cmd.hex(" ").upper())
s.reset_input_buffer()
s.write(cmd)
s.flush()

time.sleep(1.0)
data = s.read(512)
s.close()

if data:
    print(f"受信 {len(data)} バイト:")
    for i in range(0, len(data), 16):
        print("  " + data[i:i+16].hex(" ").upper())
    print()
    print("→ 無線機は応答しています。通信経路は正常です。")
else:
    print("受信ゼロ。")
    print("→ ファーム側かUSB経路の問題です。")
