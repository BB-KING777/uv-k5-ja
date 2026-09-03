# JA-K5

**Quansheng UV-K5(8) V3 / UV-K1 用の日本語ファームウェア。**
[F4HWN Edition Fusion](https://github.com/armel/uv-k1-k5v3-firmware-custom) v5.9.0 を基に、
日本語UI・短波受信（SI4732）・BK4829 レジスタ解析ツールを追加したものです。
短波受信は **実機で動作確認済み**（IOTCU V9.1B 改造基板）。

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

BK1080 のフットプリントに載せる SI4732-A10 用のドライバ・バンドプラン・受信画面。
**IOTCU V9.1B 改造基板で実機確認済み**です。

#### 受信範囲

| | 範囲 | 石 | アンテナ |
|---|---|---|---|
| LW / MW / SW | **150 kHz – 23 MHz** | SI4732 | 増設したSMA |
| FM放送 | 64 – 108 MHz | SI4732 | 元のUVアンテナ |
| VHF / UHF・エアバンド | 従来どおり | BK4829 | 元のUVアンテナ |

短波の上限が 23 MHz なのは、AN332 の `AM_TUNE_FREQ` が「149 to 23000 kHz」と
規定しているためです。データシートの「SW 2.3–26.1 MHz」とは食い違いますが、
コマンドが受け付けないので 12m / 11m / 10m は同調できません。

**26.1 – 64 MHz と 108 MHz 以上はカバーしません。**
「HF が増える」改造であって「HF〜VHF 連続」ではありません。

#### 操作

| キー | 短押し | 長押し |
|---|---|---|
| 0–9 | 周波数入力（kHz、6桁で自動確定） | — |
| MENU | 確定 / AM→LSB→USB | 前のバンド |
| ✱ | ステップ（10Hz〜10kHz） | 次のバンド |
| F | 帯域幅（6.0k〜1.0k の7段） | **感度（AGC）** |
| ↑ / ↓ | 同調 | **スキャン** |
| EXIT | 1桁消す / 画面を出る | — |

#### チップの自動選択

**周波数を打てば、聞ける方の石が選ばれます。**
メイン画面で BK4829 の下限より低い周波数を入れると SI4732 に渡って画面も切り替わり、
SI4732 画面で 23 MHz より上を入れると無線機側に戻ります。

#### 感度（AGC）

`AM_AGC_OVERRIDE`（AN332 コマンド 0x48）で AGC のゲインインデックスを 0〜37 に固定します。
`AUTO` / `DX`(0) / `NOR`(12) / `LOC`(26) / `ATT`(37) の5段。
外部アンテナで強力な中波局が近くにあると `AUTO` ではフロントエンドが飽和するので、
ここを下げます。

#### スキャン

チップ内蔵の `AM_SEEK_START` / `FM_SEEK_START` を使います。
バンド端は現在のバンド、ステップ幅は現在の同調ステップから取り、
AN332 が受け付ける値（AM は 1/5/9/10 kHz、FM は 5/10/20）に丸めます。
WRAP は切ってあるので、見つからなければバンド端で止まります。

#### バンドプラン

日本で実際に何か聞こえる区切りにしてあります（36バンド）。

- **放送** — LW / MW（9 kHz ステップ、初期値 594 kHz）/ 120m / 90m / 75m / 60m /
  49m / 41m / 31m / 25m / 22m / 19m / 16m / 13m
  ラジオNIKKEI の3波が各バンドの初期周波数（75m→3925、49m→6055、31m→9595 kHz）
- **アマチュア** — 3.5 / 7 / 10 / 14 / 18 / 21 MHz（日本の割当）
- **海上** — 4 / 6 / 8 / 12 / 16 MHz 帯
- **航空（洋上管制）** — 5628 / 6655 / 8951 / 11330 / 13300 kHz を含む5バンド
- **気象FAX** — JMH 3622.5 / 7795 / 13988.5 kHz を専用の狭いバンドで
- **TIME** — WWV / WWVH 用に 9990–10010 kHz

#### SSB

SSB パッチ（約 15.8 KB）は 2 MB の SPI フラッシュから 8 バイト単位でストリーミングします。
MCU のフラッシュも RAM も消費しません。

> [!NOTE]
> **パッチはまだ書き込む手段がありません。** フラッシュの `0x1F0000` に置く必要がありますが、
> 書き込みツールが未整備です。パッチが無い状態で SSB に入るとチップが停止するため、
> LSB / USB を選んでも自動的に AM に戻ります。

### 起動ロゴと起動音

起動ロゴ（`設定 → 画面表示 → 起動画面 → ロゴ`）は、上流では SPI フラッシュの `0x011008` から
読み込みます。そこにはベンダーのツールが書いたビットマップが入っていて、書き換えるには
フラッシュライタが要ります。本リポジトリは **1 KB の定数としてファームウェアに同梱**しました。
書き込みツール不要で、ファームと一緒に付いてきます。

また `起動画面 → SOUND` は上流では**音が鳴りません**（画面を消すだけで、対応する音が未実装）。
`AUDIO_PlayBootChime()` を足して、D5→F#5→A5→D6 の上昇アルペジオを鳴らすようにしました。
`BK4819_PlayToneRaw()` は任意の周波数を取るので、音階は `App/audio.c` の4行の表だけで決まります。

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
cmake --preset SI4732Ja && cmake --build build/SI4732Ja   # 短波受信つき（推奨）
cmake --preset FusionJa  && cmake --build build/FusionJa  # 全部入り、短波なし
cmake --preset CustomJa  && cmake --build build/CustomJa  # 軽量版
```

`SI4732Ja` は `FusionJa` に短波受信とレジスタ表示を足し、その分の容量を空けるために
Fox Hunt と RF LOG を落とした構成です。

`arm-none-eabi-gcc` / `cmake` / `ninja` が必要です。上流同梱の `./compile-with-docker.sh` も使えます。

### ビルドオプション

| オプション | 内容 | 既定 |
|---|---|---|
| `ENABLE_LANG_JA` | 日本語UI | OFF |
| `ENABLE_FIXED_NAV_K5` | 上下キーの向きを UV-K5(8) 固定 | OFF |
| `ENABLE_SI4732` | SI4732 による短波受信 | OFF |
| `ENABLE_REG_DUMP_SCREEN` | BK4829 レジスタを本体画面に表示 | OFF |

`FusionJa` / `CustomJa` は最初の2つ、`SI4732Ja` は4つとも ON です。

### フラッシュ使用量（118 KB 中）

| 構成 | 使用量 | 空き |
|---|---|---|
| **SI4732Ja** | **113,164 B** | **7,668 B** |
| FusionJa | 117,772 B | 3,060 B |
| CustomJa | 74,024 B | 46,808 B |

`FusionJa` は 97% まで来ていて、これ以上足すには何かを落とす必要があります。
`SI4732Ja` が Fox Hunt と RF LOG を落としているのはそのためです。

## 書き込み

[UV Studio](https://armel.github.io/uvstudio/) をブラウザで開き、`.bin` を選んで書き込みます。
**PTT を押しながら電源ON** でブートローダ（書き込みモード）に入ります。

CHIRP を使う場合は、上流のリリースに同梱されている **v5.9.0 用のドライバ**が必要です。

## ハードウェア改造

短波受信には、BK1080 を外して SI4732 モジュールを載せる改造が必要です。
IOTCU V9.1B の中国語インストール手順書を日本語訳したものが
[docs-ja/iotcu-v91b-guide-ja.md](docs-ja/iotcu-v91b-guide-ja.md) にあります。

> [!CAUTION]
> **8パッドの向きを間違えると電源が短絡して基板が焼けます。** 実際に1台壊しました。
> 三端子レギュレータと R500（0.5Ω の電流検出抵抗）が触れないほど発熱したら、
> ただちに電源を切ってモジュールを外してください。
> はんだ付けの**前に**、GND パッドがどれかをテスターで確認してください。

また、外装の LED 穴を拡げて SMA 座を通す際、基板側の旧 LED パッドに絶縁テープを貼らないと
SMA の芯線が短絡し、ESD ダイオードとインダクタを壊します（手順書に明記されています）。

## 既知の問題

- **S/N が常に 0 と表示される。** RSQ の応答バイト位置は AN332 で確認済み（RESP4=RSSI、
  RESP5=SNR）で、コードは正しい位置を読んでいます。チップが 0 を返している側の問題で、
  原因は未特定です。切り分け用に RSQ の生6バイトを画面最下行に出しています
- **SSB パッチの書き込み手段が無い**（上記）
- **アプリ側の USB CAT が応答しない。** ブートローダは応答するので書き込みはできますが、
  起動後のシリアル通信が繋がりません。原因未特定
- **Bluetooth はファームから制御できません。** IOTCU V9.1B の Bluetooth は基板上の
  キー検出線を直接見ている独立したチップで、操作は PTT の1回／2回／3回押しのみです。
  IOTCU 公式ファームを逆アセンブルしても、`AT+` 等の制御コードは1つも見つかりませんでした

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
