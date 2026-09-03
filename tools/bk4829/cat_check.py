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


def ping_watch(port_name, times=8):
    """0x0514 を繰り返し送る。返信を待たずに、無線機を目で見るためのモード。

    CMD_0514 は返信を送る前にバックライトを消し、PTT を 6 秒無効にする。
    コマンドが届いてパースされたかどうかは、そこで分かる。
    """
    say("バックライトが『常時点灯』以外に設定されていることを確認してください。")
    say("キーを押してバックライトを点けてから、下の送信を見ていてください。\n")

    try:
        port = serial.Serial(port_name, 38400, timeout=0.2)
    except Exception as exc:                 # noqa: BLE001
        say(f"開けません: {exc}")
        return 1

    payload = (0x0514).to_bytes(2, "little") + (4).to_bytes(2, "little") + \
              (0x12345678).to_bytes(4, "little")

    with port:
        for i in range(1, times + 1):
            port.reset_input_buffer()
            port.write(frame(payload))
            port.flush()
            time.sleep(0.4)
            got = port.read(256)
            say(f"  {i}/{times} 送信  受信 {len(got)} バイト  {describe(reply_ids(got))}")
            time.sleep(1.1)

    say("\nバックライトは消えましたか。")
    say("  消えた   -> コマンドは届いてパースされている。返信経路だけの問題")
    say("  消えない -> コマンドが届いていないか、CRC で弾かれている")
    return 0


def reset_watch(port_name):
    """0x05DD を送る。届いてパースされれば無線機は即座に再起動する。

    case 0x05DD は NVIC_SystemReset() を呼ぶだけで、タイムスタンプの照合も
    前提条件も無い。バックライトの設定に左右されないので、受信経路が
    生きているかを確かめる観測点としてはこちらのほうが確実。
    """
    say("0x05DD（リセット）を送ります。無線機を見ていてください。\n")

    try:
        port = serial.Serial(port_name, 38400, timeout=0.2)
    except Exception as exc:                 # noqa: BLE001
        say(f"開けません: {exc}")
        return 1

    payload = (0x05DD).to_bytes(2, "little") + (0).to_bytes(2, "little")

    with port:
        for i in range(1, 4):
            port.write(frame(payload))
            port.flush()
            say(f"  {i}/3 送信")
            time.sleep(2.0)

    say("\n無線機は再起動しましたか。")
    say("  再起動した   -> 受信経路もパースも CRC も通っている。返信だけの問題")
    say("  何も起きない -> フレームが無線機に届いていないか、弾かれている")
    return 0


def main():
    if len(sys.argv) < 2:
        say("使い方: python cat_check.py <ポート> [--ping] [--reset] [--sweep]\n")
        say("見えているポート:")
        for p in list_ports.comports():
            say(f"  {p.device}  {p.description}")
        return 1

    name = sys.argv[1]

    if "--ping" in sys.argv:
        return ping_watch(name)

    if "--reset" in sys.argv:
        return reset_watch(name)

    say(f"{name} を調べます。無線機は PTT を押さずに、通常起動させてください。")
    say("UV Studio を開いた Chrome のタブは閉じておいてください"
        "（WebSerial がポートを掴んだままになります）。")

    attempt(name, None, None)          # exactly what dump_regs.py does

    if "--sweep" not in sys.argv:
        say("\nDTR / RTS の総当たりは --sweep で。")
        say("0x0514 が届いているかは --ping で、無線機を見ながら確かめられます。")
    else:
        for dtr in (True, False):
            for rts in (True, False):
                # Reopening a CDC port straight after closing it blocks on
                # Windows; give the stack time to let go.
                time.sleep(1.5)
                attempt(name, dtr, rts)

    say("\n読み方")
    say("  0x0515 が返る          -> アプリの CAT は生きている")
    say("  0x0518 だけ出続ける    -> ブートローダモード。PTT を押さずに起動し直す")
    say("  どの条件でも 0 バイト  -> 送信経路かエンドポイントの問題")
    say("  バイトは来るがフレーム無し -> 難読化か CRC の食い違い")
    return 0


if __name__ == "__main__":
    sys.exit(main())
