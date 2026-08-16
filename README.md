# JA-K5

**Quansheng UV-K5(8) V3 / UV-K1 用の日本語ファームウェア。**
[F4HWN Edition Fusion](https://github.com/armel/uv-k1-k5v3-firmware-custom) v5.9.0 を基に、
日本語UI・短波受信（SI4732）・BK4829 レジスタ解析ツールを追加したものです。

> *Japanese localisation of the F4HWN firmware for the PY32F071-based Quansheng
> UV-K5(8) V3 and UV-K1, plus SI4732 HF receive support and BK4829 register
> tooling. Built on [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom)
> v5.9.0. See [Credits](#クレジット) — this is a downstream fork, not a rewrite.*

![起動画面と本体情報画面](images/ja-k5/boot.png)

---

> [!CAUTION]
> **この無線機は日本では送信できません。** 技適（技術基準適合証明）を受けていないため、
> 送信すると電波法違反になります。**受信専用機として使ってください。**
>
> 誤送信を防ぐには、メニューの **「送信ロック」** を有効にするか、
> 隠しメニュー（PTT＋上サイドキーを押しながら電源ON）の **「送信帯域」→「送信 全面禁止」** を
> 設定してください。
>
> 受信は自由ですが、電波法59条により、傍受した通信の秘密を漏らしたり窃用することは禁じられています。

> [!WARNING]
> 上流と同じく、**このファームウェアに保証はありません。** 文鎮化する可能性もあります。
> 書き込む前に [UV Studio](https://armel.github.io/uvstudio/#dump-calib) で
> **キャリブレーションデータを必ずバックアップ**してください。

---

## 対応機種

**PY32F071 を積んだ UV-K5(8) V3 と UV-K1 専用です。**
旧来の DP32G030 版（UV-K5 V1 / V2 / K6）に書き込むと確実に文鎮化します。

見分け方は、USB-C を PC につないで COM ポートが生えるかどうか。
生えれば V3（PY32 は USB デバイス機能を内蔵）、充電だけなら V1/V2 です。

## 追加した機能

### 日本語UI (`ENABLE_LANG_JA`)

![日本語メニュー](images/ja-k5/menu-ja.png)

メニュー名95項目、設定値150行、カテゴリ名、サイドキー機能名を日本語化しています。

フォントは **美咲ゴシック8x8**（門真なむ氏）。firmware が実際に使う文字だけを抜き出して
埋め込んでいるので、日本語対応のコストは **+3.8 KB** に収まっています。
描画は `UI_PrintString` 系を UTF-8 対応にしただけで、**ASCII の描画結果は従来と1ドットも変わりません**
（`ENABLE_LANG_JA=OFF` でビルドすると上流とバイナリがほぼ一致することを確認済み）。

実装の詳細は [README_JA.md](README_JA.md) を参照してください。

### 上下キーの向きを固定 (`ENABLE_FIXED_NAV_K5`)

上流は初期値が UV-K1 向け（左右キー）のため、UV-K5(8) では**周波数の増減が逆**になります。
本来はメニューの `SetNav` で直せますが、これは隠しメニューにあって気づきにくく、
さらに `ENABLE_FEAT_F4HWN_RESCUE_OPS` が無効なビルドでは**設定が保存されません**（上流の不具合）。

このオプションは `SET_NAV` を UV-K5(8) 固定にし、該当メニューを消します。焼いた瞬間から正しい向きです。

### 短波受信 (`ENABLE_SI4732`)

![SI4732 受信画面](images/ja-k5/si4732.png)

BK1080 のフットプリントに載せる SI4732-A10 モジュール用のドライバ・バンドプラン・受信画面。

- **LW 153–279 kHz / MW 522–1710 kHz / SW 2.3–26.1 MHz / FM 76–108 MHz**
- AM / LSB / USB / FM、帯域7段、BFO、25バンドのバンドプラン
  （160m・80m・40m は日本の割当に合わせた帯域端）
- SSB パッチ（15.8 KB）は 2 MB の SPI フラッシュから8バイト単位でストリーミング。
  MCU のフラッシュも RAM も消費しません
- コストは **+3.1 KB**

> [!IMPORTANT]
> **実機未検証です。** モジュール未着のため、コンパイルと画面描画の確認までしかできていません。
> I2C のタイミング、パッチ転送、アンテナ経路、音声レベルはいずれも未確認です。

SI4732 は 26.1〜64 MHz と 108 MHz 以上をカバーしません。エアバンドや 144/430 は
従来どおり BK4829 が担当します。**「HF が増える」改造であって「HF〜VHF 連続」ではありません。**

### BK4829 レジスタ表示 (`ENABLE_REG_DUMP_SCREEN`)

![レジスタ画面](images/ja-k5/regdump.png)

BK4829 の全128レジスタを本体画面に表示します。1〜2ページ目は、firmware が一度も書かない
8本のレジスタをデータシートの既定値と自動照合して OK / NG を出します。

これは Beken の「BK4829 Registers Table」が
[BK4819 の文書の使い回しではないか](https://github.com/armel/uv-k1-k5v3-firmware-custom/discussions/36)
という疑義を実機で確かめるために作ったものです。結果は **8/8 一致**、さらに
**`REG_00` が `0x4829` を返す**ことが分かりました（表の `48x9` はファミリー共通の表記でした）。

CAT 経由で読む [`tools/bk4829/dump_regs.py`](tools/bk4829/dump_regs.py) も同梱しています。

## ビルド

```sh
cmake --preset FusionJa && cmake --build build/FusionJa   # 全部入り
cmake --preset CustomJa && cmake --build build/CustomJa   # 軽量版
```

`arm-none-eabi-gcc` / `cmake` / `ninja` が必要です。上流同梱の `./compile-with-docker.sh` も使えます。

### ビルドオプション

| オプション | 内容 | 既定 |
|---|---|---|
| `ENABLE_LANG_JA` | 日本語UI | OFF |
| `ENABLE_FIXED_NAV_K5` | 上下キーの向きを UV-K5(8) 固定 | OFF |
| `ENABLE_SI4732` | SI4732 による短波受信（**実機未検証**） | OFF |
| `ENABLE_REG_DUMP_SCREEN` | BK4829 レジスタを本体画面に表示 | OFF |

プリセット `FusionJa` / `CustomJa` は最初の2つが ON です。

### フラッシュ使用量（118 KB 中）

| 構成 | 使用量 | 空き |
|---|---|---|
| Fusion（英語・上流相当） | 112,576 B | 8,256 B |
| **FusionJa** | **116,552 B** | **4,280 B** |
| CustomJa | 73,668 B | 47,164 B |

Fusion は残りが少ないので、機能を足すときは何かを削る必要があります。
実測値は [監査結果](README_JA.md) を参照してください。

## 書き込み

[UV Studio](https://armel.github.io/uvstudio/) をブラウザで開き、`.bin` を選んで書き込みます。
**PTT を押しながら電源ON** でブートローダ（書き込みモード）に入ります。

CHIRP を使う場合は、上流のリリースに同梱されている **v5.9.0 用のドライバ**が必要です。

## クレジット

このファームウェアは、以下の積み重ねの上に成り立っています。

- **[DualTachyon](https://github.com/DualTachyon/uv-k5-firmware)** — UV-K5 のオープンソースファームウェアを最初に公開
- **[egzumer](https://github.com/egzumer/uv-k5-firmware-custom)** — 機能を大きく拡張したカスタム版
- **[armel / F4HWN](https://github.com/armel/uv-k5-firmware-custom)** — F4HWN Edition
- **[armel / F4HWN と muzkr](https://github.com/armel/uv-k1-k5v3-firmware-custom)** — PY32F071（UV-K1 / UV-K5 V3）への移植。**本リポジトリの直接の派生元**

上流の README は [README_F4HWN.md](README_F4HWN.md) にそのまま残してあります。
機能の大半は上流のものです。**このリポジトリが追加したのは上記4項目だけ**だと考えてください。

本体の「本体情報」画面に、派生元のバージョン（`F4HWN 5.9.0`）が表示されます。

## ライセンス

Apache License 2.0。DualTachyon 以来の著作権表示を引き継いでいます。詳細は
[LICENSE](LICENSE) と [NOTICE](NOTICE) を参照してください。

同梱の **美咲フォント**（`tools/lang_ja/misaki_gothic.bdf` と、そこから生成した
`App/font_ja.c`）は Apache 2.0 の対象外で、それ自身のライセンスで再配布しています。

> These fonts are free softwares. Unlimited permission is granted to use, copy,
> and distribute it, with or without modification, either commercially and
> noncommercially. THESE FONTS ARE PROVIDED "AS IS" WITHOUT WARRANTY.
>
> — 美咲フォント (c) 門真なむ https://littlelimit.net/misaki.htm
