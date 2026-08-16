#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
dump_regs.py — Read every BK4829 register out of a live radio and compare it
against the defaults published in Beken's register table.

Why: the table circulating as "BK4829 Registers Table" (DRT01-230606-C01) has
been questioned as possibly being the BK4819 document under a new title. The
registers the firmware never writes still hold their reset defaults, so reading
them back from real silicon settles whether the document describes this chip.

Requires firmware built with -DENABLE_UART_RW_BK_REGS=ON.

Usage:
    python3 dump_regs.py COM7          # Windows
    python3 dump_regs.py /dev/ttyACM0  # Linux
"""

import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required:  pip install pyserial")

OBFUSCATION = bytes([0x16, 0x6C, 0x14, 0xE6, 0x2E, 0x91, 0x0D, 0x40,
                     0x21, 0x35, 0xD5, 0x40, 0x13, 0x03, 0xE9, 0x80])

# Section 2 of the register table, "Registers' Default Value".
# None = the table leaves the cell blank.
DEFAULTS = {
    0x05: 0x7819, 0x10: 0x0038, 0x11: 0x025A, 0x12: 0x037B, 0x13: 0x03DE,
    0x14: 0x0000, 0x15: 0x8005, 0x16: 0x8080, 0x17: 0x7839, 0x18: 0x4525,
    0x19: 0x9041, 0x1A: 0x5850, 0x1B: 0x2200, 0x1C: 0x0000, 0x1D: 0x2AAB,
    0x1E: 0x4C51, 0x1F: 0x5454, 0x20: 0x0000, 0x21: 0x06D8, 0x22: 0x4D08,
    0x23: 0x8410, 0x24: 0x8C5E, 0x25: 0xC1BA, 0x26: 0x0000, 0x27: 0x0000,
    0x28: 0x0A00, 0x29: 0xA600, 0x2A: 0x5109, 0x2B: 0x0000, 0x2C: 0x3462,
    0x2D: 0x4B18, 0x2E: 0x9608, 0x2F: 0x98D8, 0x30: 0x0000, 0x31: 0x0000,
    0x32: 0x0244, 0x33: 0xFF00, 0x34: 0x0000, 0x35: 0x0000, 0x36: 0x003F,
    0x37: 0x1F00, 0x38: 0x3A98, 0x39: 0x0271, 0x3A: 0x049A, 0x3B: 0x5880,
    0x3C: 0x4F88, 0x3D: 0x0000, 0x3E: 0x8E6A, 0x3F: 0x0000, 0x40: 0x34D0,
    0x41: 0x81C3, 0x42: 0x6B5A, 0x43: 0x4048, 0x44: 0x9009, 0x45: 0x31A9,
    0x46: 0xA050, 0x47: 0x6140, 0x48: 0x338F, 0x49: 0x2830, 0x4A: 0x5448,
    0x4B: 0x710D, 0x4C: 0xA520, 0x4D: 0xA020, 0x4E: 0x6F08, 0x4F: 0x2F2E,
    0x50: 0x0000, 0x51: 0x0000, 0x52: 0x028F, 0x53: 0x1130, 0x54: 0x9009,
    0x55: 0x31A9, 0x56: 0x1021, 0x57: 0x0000, 0x58: 0x0000, 0x59: 0x0000,
    0x5A: 0x85CF, 0x5B: 0xAB45, 0x5C: 0x56F9, 0x5D: 0x0F00, 0x5E: 0x3044,
    0x70: 0x0000, 0x71: 0x8517, 0x72: 0x2854, 0x73: 0x4682, 0x74: 0xF50B,
    0x75: 0xF50B, 0x76: 0xE380, 0x77: 0xA8FF, 0x78: 0x4846, 0x79: 0x4040,
    0x7A: 0x889A, 0x7B: 0xAE34, 0x7C: 0x8000, 0x7D: 0xE51C, 0x7E: 0x302E,
}

# Registers the firmware never writes. These are the ones that matter: they
# should still read back as the reset default, so they are a direct test of
# whether the document matches this silicon.
UNTOUCHED = {0x0A, 0x1A, 0x2E, 0x34, 0x35, 0x44, 0x45, 0x62, 0x66, 0x6E,
             0x74, 0x75}


def crc16(data):
    """CRC-16/XMODEM, matching CRC_Calculate() in App/driver/crc.c."""
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


def frames(raw):
    """Yield every valid frame in the buffer.

    Framing:  AB CD | size(LE16) | payload(size) XOR-obfuscated | 2 bytes | DC BA

    Those two trailing bytes are NOT a CRC on the way back. SendReply_VCP()
    in App/app/uart.c writes  Obfuscation[size % 16] ^ 0xFF  there, i.e. a
    plain 0xFFFF placeholder once de-obfuscated. Only commands going TO the
    radio carry a real CRC-16, because UART_IsCommandAvailable() checks it.
    Validating replies against a CRC therefore throws away every single one.
    """
    pos = 0
    while True:
        start = raw.find(b"\xAB\xCD", pos)
        if start < 0:
            return

        pos = start + 2
        if len(raw) < start + 8:
            return

        size = int.from_bytes(raw[start + 2:start + 4], "little")
        if size > 512:
            continue

        end = start + 4 + size + 2
        if len(raw) < end + 2:
            continue

        if raw[end:end + 2] != b"\xDC\xBA":      # footer ID 0xBADC
            continue

        body = bytearray(raw[start + 4:end])
        for i in range(len(body)):
            body[i] ^= OBFUSCATION[i % 16]

        payload = bytes(body[:size])
        trailer = int.from_bytes(body[size:size + 2], "little")

        # accept the reply placeholder, and also a genuine CRC
        if trailer == 0xFFFF or trailer == crc16(payload):
            yield payload


def transact(port, payload, want_id, timeout=0.6, debug=False):
    port.reset_input_buffer()
    port.write(frame(payload))
    port.flush()

    deadline = time.time() + timeout
    buffer = b""

    while time.time() < deadline:
        chunk = port.read(256)
        if chunk:
            buffer += chunk
        for reply in frames(buffer):
            if int.from_bytes(reply[:2], "little") == want_id:
                return reply
        time.sleep(0.005)

    if debug and buffer:
        print(f"   [debug] 受信 {len(buffer)} バイト: {buffer[:64].hex(' ')}")
    elif debug:
        print("   [debug] 受信ゼロ")

    return None


# The application answers 0x0514 with 0x0515. A radio sitting in flashing mode
# never does: its bootloader just beacons 0x0518 over and over, and knows none
# of the application commands.
REPLY_APP        = 0x0515
REPLY_BOOTLOADER = 0x0518


def hello(port, debug=False):
    """CMD 0x0514. Returns (reply_id, payload) for whatever frame comes back."""
    payload = (0x0514).to_bytes(2, "little") + (4).to_bytes(2, "little") + \
              (0x12345678).to_bytes(4, "little")

    port.reset_input_buffer()
    port.write(frame(payload))
    port.flush()

    deadline = time.time() + 1.2
    buffer = b""

    while time.time() < deadline:
        chunk = port.read(256)
        if chunk:
            buffer += chunk
        for reply in frames(buffer):
            return int.from_bytes(reply[:2], "little"), reply
        time.sleep(0.005)

    if debug and buffer:
        print(f"   [debug] 受信 {len(buffer)} バイト: {buffer[:64].hex(' ')}")

    return None, None


def read_register(port, reg, debug=False):
    payload = (0x0601).to_bytes(2, "little") + (1).to_bytes(2, "little") + bytes([reg])
    reply = transact(port, payload, 0x0601, debug=debug)

    if reply is None or len(reply) < 7:
        return None

    return int.from_bytes(reply[5:7], "little")


def list_ports():
    try:
        from serial.tools import list_ports as lp
    except ImportError:
        return []
    return list(lp.comports())


def open_port(name, attempts=3):
    """Open the CDC port.

    Keep this as close to a bare pyserial open as possible. Setting dsrdtr /
    rtscts / write_timeout makes pyserial push extra fields through
    SetCommState, and on this CDC device that call either times out
    (ERROR_SEM_TIMEOUT) or blocks forever. The plain constructor is the form
    that has actually been observed to open.

    DTR does not need setting either: pyserial already asserts it on open,
    which is what the firmware's cdc_acm_data_send_with_dtr() waits for.
    """
    last = None

    for attempt in range(attempts):
        try:
            print(f"{name} を開いています ...", end=" ", flush=True)
            port = serial.Serial(name, 38400, timeout=0.3)
            print("OK")
            time.sleep(0.4)
            return port
        except Exception as exc:     # noqa: BLE001 - surface whatever Windows says
            last = exc
            print("失敗")
            print(f"   {exc}")
            time.sleep(1.5)

    print()
    print("ポートを開けませんでした。USB-C を一度抜き差ししてから再実行してください。")
    print("（前回 Ctrl-C で中断していると、Windows 側にハンドルが残って開けなくなります）")
    raise last


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    debug = "--debug" in sys.argv

    ports = list_ports()
    if ports:
        print("検出されたシリアルポート:")
        for p in ports:
            print(f"  {p.device:<8} {p.description}")
        print()

    if not args:
        sys.exit(__doc__)

    if ports and not any(p.device.upper() == args[0].upper() for p in ports):
        print(f"※ {args[0]} は今このPCに存在しません。上の一覧から選び直してください。")
        print("  無線機のUSBを挿し直すと復活することがあります。")
        return

    port = open_port(args[0])

    with port:
        print("無線機と接続中 ...", end=" ", flush=True)

        reply_id = None
        for _ in range(3):
            reply_id, _ = hello(port, debug)
            if reply_id is not None:
                break
            time.sleep(0.4)

        if reply_id is None:
            print("応答なし")
            print()
            print("チェック項目:")
            print("  - ポート番号は合っているか（上の一覧を参照）")
            print("  - ★ UV Studio を開いた Chrome のタブが残っていないか")
            print("    （WebSerial はタブを閉じるまでポートを掴み続けます）")
            print("  - 充電専用ではなくデータ用の USB-C ケーブルか")
            print("  - --debug を付けて再実行すると受信生データが出ます")
            return

        if reply_id == REPLY_BOOTLOADER:
            print("ブートローダでした")
            print()
            print("無線機が書き込み(フラッシュ)モードのままです。")
            print("このモードのブートローダは 0x0518 を周期的に送るだけで、")
            print("レジスタ読み出しを含むアプリのコマンドには一切応答しません。")
            print()
            print("対処: PTT を押さずに電源を入れ直し、通常画面が出てから再実行してください。")
            return

        if reply_id != REPLY_APP:
            print(f"応答あり(ID 0x{reply_id:04X})")
        else:
            print("OK")

        values = {}
        for reg in range(0x80):
            values[reg] = read_register(port, reg, debug)
            if reg % 16 == 15:
                print(f"  読み出し {reg + 1}/128", end="\r", flush=True)
        print(" " * 30, end="\r")

    if not any(v is not None for v in values.values()):
        print("接続はできましたが、レジスタ読み出しコマンドに応答がありません。")
        print("ENABLE_UART_RW_BK_REGS を有効にしたファームか確認してください。")
        return

    print(f"{'REG':<6}{'読値':<9}{'既定値':<9}{'一致':<7}判定")
    print("-" * 58)

    checked = matched = 0

    for reg in range(0x80):
        got = values[reg]
        if got is None:
            continue

        expected = DEFAULTS.get(reg)
        note = ""

        if reg in UNTOUCHED and expected is not None:
            checked += 1
            ok = (got == expected)
            matched += ok
            note = "★未書込レジスタ — 一致すべき" if ok else "★不一致 — 要調査"
            mark = "OK" if ok else "NG"
        elif expected is None:
            mark = "-"
            note = "表に既定値なし"
        else:
            mark = "OK" if got == expected else "差分"
            note = "ファームが書き換え済み" if got != expected else ""

        print(f"0x{reg:02X}  0x{got:04X}   "
              f"{('0x%04X' % expected) if expected is not None else '  --  '}   "
              f"{mark:<6} {note}")

    print("-" * 58)
    print(f"未書込レジスタの一致: {matched}/{checked}")
    if checked and matched == checked:
        print("→ 表の既定値は実チップと一致。この文書は BK4829 のものとして信頼できる。")
    elif checked:
        print("→ 不一致あり。文書が BK4819 由来である疑いを裏付ける可能性。")


if __name__ == "__main__":
    main()
