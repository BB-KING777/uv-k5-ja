#!/usr/bin/env python3
"""アプリ側の CAT が応答するかを、条件を変えながら確かめる。

無応答の原因を切り分けるための道具。以下を順に見る。

  1. 何も送らずに 1 秒聞く   -> ブートローダは 0x0518 を周期的に吐く
  2. 0x0514 を投げて聞く     -> アプリなら 0x0515 が返る
  3. DTR / RTS の 4 通りで繰り返す

生バイトをそのまま出すので、「フレームとして壊れている」のか
「そもそも 1 バイトも来ない」のかが区別できる。

    python cat_check.py COM19
"""
import sys
import time

import serial
from serial.tools import list_ports

def say(*args):
    """Print and flush, so a hang is visible at the line it happened on."""
    print(*args, flush=True)


OBFUSCATION = bytes([0x16, 0x6C, 0x14, 0xE6, 0x2E, 0x91, 0x0D, 0x40,
                     0x21, 0x35, 0xD5, 0x40, 0x13, 0x03, 0xE9, 0x80])


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def frame(payload):
    body = bytearray(payload) + crc16(payload).to_bytes(2, "little")
    for i in range(len(body)):
        body[i] ^= OBFUSCATION[i % 16]
    return b"\xAB\xCD" + len(payload).to_bytes(2, "little") + bytes(body) + b"\xDC\xBA"


def reply_ids(raw):
    """バッファ内のフレームの ID を拾う。CRC は見ない。"""
    out, pos = [], 0
    while True:
        start = raw.find(b"\xAB\xCD", pos)
        if start < 0 or len(raw) < start + 8:
            return out
        pos = start + 2
        size = int.from_bytes(raw[start + 2:start + 4], "little")
        end = start + 4 + size + 2
        if size > 512 or len(raw) < end + 2 or raw[end:end + 2] != b"\xDC\xBA":
            continue
        body = bytearray(raw[start + 4:end])
        for i in range(len(body)):
            body[i] ^= OBFUSCATION[i % 16]
        out.append(int.from_bytes(body[:2], "little"))


def describe(ids):
    if not ids:
        return "フレームなし"
    names = {0x0515: "0x0515 アプリの版数応答", 0x0518: "0x0518 ブートローダのビーコン"}
    return " / ".join(names.get(i, f"0x{i:04X} 不明") for i in dict.fromkeys(ids))


def attempt(port_name, dtr, rts):
    if dtr is None:
        say("\n--- 素で開く（信号線を触らない）---")
    else:
        say(f"\n--- DTR={int(dtr)} RTS={int(rts)} ---")

    say("  ポートを開いています ...")
    try:
        port = serial.Serial(port_name, 38400, timeout=0.2)
    except Exception as exc:                 # noqa: BLE001
        say(f"  開けません: {exc}")
        say("  → UV Studio を開いた Chrome のタブが残っていませんか。"
            "WebSerial はポートを排他的に掴みます。")
        return

    with port:
        say("  開けました")

        # Setting these calls EscapeCommFunction on Windows, which has been
        # seen to block on this CDC device. Never let it take the run down.
        if dtr is not None:
            for name, value in (("dtr", dtr), ("rts", rts)):
                try:
                    setattr(port, name, value)
                except Exception as exc:     # noqa: BLE001
                    say(f"  {name} を {int(value)} にできません: {exc}")

        time.sleep(0.2)
        port.reset_input_buffer()

        say("  無送信で 1 秒聞きます ...")
        quiet = b""
        deadline = time.time() + 1.0
        while time.time() < deadline:
            quiet += port.read(256)
        say(f"  無送信で 1 秒: {len(quiet):4d} バイト  {describe(reply_ids(quiet))}")
        if quiet:
            say(f"    {quiet[:48].hex(' ')}")

        payload = (0x0514).to_bytes(2, "little") + (4).to_bytes(2, "little") + \
                  (0x12345678).to_bytes(4, "little")
        say("  0x0514 を送ります ...")
        port.reset_input_buffer()
        port.write(frame(payload))
        port.flush()

        answer = b""
        deadline = time.time() + 1.2
        while time.time() < deadline:
            answer += port.read(256)
        say(f"  0x0514 送信後 : {len(answer):4d} バイト  {describe(reply_ids(answer))}")
        if answer:
            say(f"    {answer[:48].hex(' ')}")


def main():
    if len(sys.argv) < 2:
        say("使い方: python cat_check.py <ポート>\n")
        say("見えているポート:")
        for p in list_ports.comports():
            say(f"  {p.device}  {p.description}")
        return 1

    name = sys.argv[1]
    say(f"{name} を調べます。無線機は PTT を押さずに、通常起動させてください。")
    say("UV Studio を開いた Chrome のタブは閉じておいてください"
        "（WebSerial がポートを掴んだままになります）。")

    attempt(name, None, None)          # exactly what dump_regs.py does

    for dtr in (True, False):
        for rts in (True, False):
            attempt(name, dtr, rts)

    say("\n読み方")
    say("  0x0515 が返る          -> アプリの CAT は生きている")
    say("  0x0518 だけ出続ける    -> ブートローダモード。PTT を押さずに起動し直す")
    say("  どの条件でも 0 バイト  -> 送信経路かエンドポイントの問題")
    say("  バイトは来るがフレーム無し -> 難読化か CRC の食い違い")
    return 0


if __name__ == "__main__":
    sys.exit(main())
