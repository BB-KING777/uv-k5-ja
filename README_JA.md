# 日本語UI の実装ノート

プロジェクトの概要・警告・ビルド方法は [README.md](README.md) を参照してください。
ここは `ENABLE_LANG_JA` の中身の説明です。

## 何が日本語になるか（ENABLE_LANG_JA）

F4HWN (UV-K1 / UV-K5 V3 = PY32F071版) のメニューを日本語表示にするビルドオプションです。
本家のリソースには手を入れず、`ENABLE_LANG_JA` を OFF にすれば従来どおり英語で
1バイトも変わらないバイナリが出ます。

## 何が日本語になるか

- メニュー名（95項目すべて）
- 各メニューの設定値（150行）
- メニューのカテゴリ名（Fusion の分類表示）
- サイドキーに割り当てる機能の名前

メイン画面・ステータスバー・スペアナ等は英語のままです。ここは 3x5 ドットの
極小フォントで描かれていて、日本語を入れると読めなくなるためです。

## 表示の仕組み

- フォントは **美咲ゴシック 8x8**（門真なむ氏、M+ FONTS ライセンス相当の自由な
  ライセンス。商用・改変・再配布いずれも可）。
- firmware が実際に使っている文字だけを `tools/lang_ja/misaki2c.py` で抜き出して
  C の配列にしています。現状 208 文字・2,080 バイト。
- 描画は `UI_PrintString` / `UI_PrintStringSmall` を UTF-8 対応にしただけで、
  ASCII の描画結果は従来と完全に同一です。
- 大きい文字が要求される場所（メニューの値の欄など）では、幅に収まる場合のみ
  8x8 グリフを縦横 2 倍に拡大して 16x16 で描きます。収まらない場合は 8x8 のまま
  16px の帯の中央に描きます。
- メニュー左カラムは 48px = 日本語 6 文字が上限。選択中の項目は
  文字サイズではなく白黒反転で示します。

## ビルド

```sh
cmake --preset CustomJa && cmake --build build/CustomJa   # 軽量版
cmake --preset FusionJa && cmake --build build/FusionJa   # 全部入り
```

既存のプリセットに `-DENABLE_LANG_JA=ON` を付けても同じです。

### フラッシュ使用量 (118KB 中)

| プリセット | 英語 | 日本語 | 差分 |
|---|---|---|---|
| Custom | 69,852 B (57.8%) | 73,676 B (61.0%) | +3,824 B |
| Fusion | 112,524 B (93.1%) | 116,452 B (96.4%) | +3,928 B |

Fusion は残り 4KB を切るので、機能を足すときは注意してください。

## 訳語を直したいとき

1. `App/ui/menu_ja.inc` を編集する（英語版は `App/ui/menu.c` 側にそのまま残して
   あります。項目数は必ず一致させること。`ARRAY_SIZE` で設定値の範囲を決めて
   いるため、1 個ずれると設定範囲が壊れます）
2. `tools/lang_ja/build_font.sh` を実行してフォントを再生成する
   （新しい漢字を使った場合に必要。美咲の BDF が要る）
3. 再ビルド

`tools/lang_ja/sim/render.sh` を実行すると、実機に焼かずに PC 上で
firmware と同じ描画ルーチンを動かして画面を PNG に書き出せます。
レイアウト崩れの確認用です。

## 制約

- メニュー名は 6 文字（48px）まで。`tools` のチェックを通してから使ってください。
- 設定値の 1 行は 9 文字（78px）まで。超えると右端で切り捨てられます。
- 美咲フォントは 7x7 ドットに漢字を収めているので、画数の多い字は潰れます。
  訳語はなるべく画数の少ない語を選んでいます。

## ライセンス

firmware 本体は Apache License 2.0（DualTachyon / egzumer / F4HWN / muzkr の
著作権表示を引き継ぎます）。埋め込みフォントは美咲フォント:

> These fonts are free softwares. Unlimited permission is granted to use, copy,
> and distribute it, with or without modification, either commercially and
> noncommercially. THESE FONTS ARE PROVIDED "AS IS" WITHOUT WARRANTY.

https://littlelimit.net/misaki.htm
