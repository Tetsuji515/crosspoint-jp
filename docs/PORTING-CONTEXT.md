# 移植コンテキスト(crosspoint-reader-cjk → crosspoint-jp)

日付: 2026-07-14
指示書: `D:\X4\xteink_x4_jp_base_migration_instruction.md`

このリポジトリ(`Tetsuji515/crosspoint-jp`、upstream=`zrn-ns/crosspoint-jp`)は、X4カスタムFW開発の**新しい開発正本**である(STEP 3でGo判断済み、`docs/dev-notes/crosspoint-jp-x4-eval.md` 参照)。

## 旧ベースからの持ち込みドキュメントと前提の変更

`docs/design/xteink_x4_basic_design.md` と `docs/dev-notes/` の以下は、旧ベース
**crosspoint-reader-cjk(`Tetsuji515/crosspoint-reader-cjk`、タグ `phase1-complete`)** 時代の成果物のコピーである。
コード参照(ファイルパス・クラス名・行番号)は旧ベースのものなので、本リポジトリのコードと突き合わせる際は読み替えが必要:

| ドキュメント | 内容 | 新ベースでの扱い |
|---|---|---|
| `dev-notes/home-launcher-survey.md` | 旧ベースのActivity構造調査 | 4-1で本リポジトリ版の差分表を作る際の比較元 |
| `dev-notes/clock-sync-survey.md` | X4のRTC非搭載の実機検証(スリープ=完全電源断) | **ハードウェア知見として有効**。ClockSyncは凍結中 — 移植しない |
| `dev-notes/mem-investigation.md` | 低メモリクラッシュ調査(Back二重発火・断片化) | 2バグとも新ベースで再現せず(評価#7)。再現した場合のみ新ベースに合わせ再実装 |
| `dev-notes/reading-font-format-survey.md` | `.epdf` 形式の互換性調査 | **新ベースは `.cpfont` 形式のため直接は適用不可**。フォントDL機能で足りる見込み(評価#3) |
| `design/xteink_x4_basic_design.md` | 要件・基本設計 | 機能要件は有効。ベースFW前提の記述は crosspoint-jp に読み替え |

## 旧ベース資産の所在

- 旧リポジトリ: `D:\X4\crosspoint-reader-cjk`(保全、アーカイブしない。戻り先かつ設計ドキュメント正本)
- 旧FWバイナリ: `D:\X4\backup\phase1-complete\`(bootloader/firmware/partitions.bin)
- 純正FW: `D:\X4\backup\xteink_x4_stock_backup.bin`
- JP読書フォント生成(旧`.epdf`用): 旧リポジトリ `scripts/font/`

## 運用原則(旧ベースから継続)

- 機能ごと `feature/*` ブランチ、`pio run` 確認後コミット
- サブモジュール `open-x4-sdk`(=`open-x4-epaper/community-sdk`)は無変更原則
- Windows環境では `PYTHONUTF8=1 PYTHONIOENCODING=utf-8` を付与(cp932対策)
- **ClockSync は凍結中 — 移植・再実装しない**(解除判断は本人)
