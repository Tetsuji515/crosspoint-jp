# crosspoint-jp X4 評価記録

日付: 2026-07-14
指示書: `D:\X4\xteink_x4_jp_base_migration_instruction.md` STEP 2
評価者: 実機操作=本人 / ログ解析・記録=Claude Code

---

## 2-1. 入手とビルド

- フォーク: `https://github.com/Tetsuji515/crosspoint-jp`(`gh repo fork --clone=false`で作成)
- クローン: `D:\X4\crosspoint-jp`(`--recursive`、サブモジュール `open-x4-sdk` = `community-sdk` @ `a64a3c2`)
- リモート: `origin`=自分のフォーク、`upstream`=`zrn-ns/crosspoint-jp`(現行リポジトリと同じ規約)
- **リリース済みビルド**: v0.1.7(2026-04-30、Latest)に `bootloader.bin` / `firmware.bin` / `partitions.bin` あり。ただしX4での動作確認はupstream未実施(README明記)
- **ビルド環境の差分**: `platformio.ini` は現行(crosspoint-reader-cjk)と同系。`board = esp32-c3-devkitm-1`、16MB flash、環境は `default` / `gh_release` 等。**X4専用envは存在せず、X3/X4はランタイム判定(`deviceIsX3()`、HalGPIO)の共通バイナリ**
- ローカルビルド(`pio run`、Windows、PYTHONUTF8=1): **SUCCESS(545秒、警告での中断なし)**。`firmware.bin` 6,014,272バイト生成。環境設定・ボード指定の変更は一切不要(クローン直後のままビルド可)

## 2-2. 書き込みと復帰手段

- 戻し手段の確保(書き込み前に完了):
  - 現行FW: `phase1-complete` タグのビルド成果物を `D:\X4\backup\phase1-complete\`(bootloader/firmware/partitions.bin)に保管済み
  - 純正FW: `D:\X4\backup\xteink_x4_stock_backup.bin`(16MB、健在)
  - 復元手順: esptool(要 `PYTHONUTF8=1`、COMポート再列挙の数秒待ち)
- crosspoint-jp書き込み: **完了(2026-07-14)**。`pio run -t upload`(COM3、PYTHONUTF8=1)で6,014,272バイト書き込み・ハッシュ検証パス
- 起動ログ(シリアル、リセット後の全シーケンス採取):
  - **SSD1677コントローラ初期化OK、フレームバッファ48,000バイト(=800×480、X4パネル)を正しく駆動**
  - Boot→Homeアクティビティ遷移正常、half/fast refreshとも動作(894ms/397ms)
  - SDカード読み取りOK(既存の `.crosspoint` から最近の本6冊をロード)
  - `[ERR] [SDREG] Fonts directory not found: /.crosspoint/fonts` → .cpfontフォント未導入のため想定内(現行の `/fonts/*.epdf` とはディレクトリも形式も別)
  - ESP-IDF復元時刻を使用(`Using ESP-IDF restored time`)→ 時刻処理の方式として記録(ClockSync凍結中)
  - **空きヒープ 119,792バイト / 最大連続ブロック 65,524バイト** — 現行FW(90〜97KB)より良好。クラッシュ・エラーなし、低電力モード移行も正常

## 2-3. 評価チェックリスト(実機)

| # | 項目 | 結果 | メモ |
|---|---|---|---|
| 1 | 起動・基本動作 | **○** | Boot→Home遷移・描画・SD読込・低電力移行すべて正常(ログ確認)。ボタン操作・画面遷移も本人確認で問題なし。ヒープ119KBと良好 |
| 2 | 日本語UI | **○** | 本人確認。文字化け・表示崩れなし |
| 3 | 日本語フォント導入 | **○** | **デバイス直接DLがX4で動作、全フォントのDL完了**(本人確認)。フォント形式は `.cpfont`(`CPFONT\0` マジック、`FontInstaller.h`)= 本家v1.2系。現行の `.epdf` パイプライン(`ttf_to_epdfont.py`)はそのままでは使えないが、DL機能で足りる可能性が高い |
| 4 | 横書き読書 | 一部確認 | 表示・ページ送りは実用レベル(本人確認)。**前ページの残像(ゴースト)が気になるとの所見** → 設定「リフレッシュ頻度」(デフォルト15ページごとにフルリフレッシュ、`refreshFrequency`)を短くして再評価予定 |
| 5 | 縦書き読書(主目的) | **○** | テストEPUB(`test_book_vertical.epub`)で本人確認。縦書き表示・字形変換・ルビとも問題なし |
| 6 | フォントサイズ変更(主目的) | **○** | 読書中のサイズ変更が動作(本人確認) |
| 7 | メモリ挙動 | **○(旧2バグとも再現せず)** | 設定→Back→ホームで本の勝手な再オープンなし。ファイル転送→読書→ホームの断片化シナリオでもクラッシュなし。**ただしログ上はメモリ逼迫の兆候あり(下記「#7詳細」)** |
| 8 | ファイル転送 | **○** | WiFi接続・WebUIからのEPUBアップロードとも問題なし(本人確認) |
| 9 | KOReader同期 | スキップ(環境なし) | 旧ベースでも未確認のため比較基準として対等。移行判断には影響させない |
| 10 | スリープ画面 | **○** | スリープ画像の表示を確認(本人確認)。X4はRTC非搭載だがスリープ画面自体は動作 |
| 11 | EPUB内画像・テーブル | **○** | テストEPUB第二章の画像(PNG)・テーブルとも表示(本人確認) |
| 12 | 安定性(30分) | **○** | 全機能巡回(WebServer→FileBrowser→Settings→Reader→縦書き読書)でクラッシュ・フリーズ・予期せぬ再起動ゼロ(シリアルログで裏付け: セッション中のbootログ0回) |

### #7詳細: シリアルログ解析(30分採取、16,440行、MEMサンプル73点)

- ヒープ推移: Free 最小38,640 / 最大140,788バイト。**起動来最小(MinFree)は3,624バイト**まで低下(WiFi+Webサーバー動作中と推定)— かなり際どいが枯渇はせず
- 最大連続ブロック(MaxAlloc)は27,636バイトまで低下(断片化はこのFWでも起きる)
- **`[ERR] [GFX] !! Failed to allocate BW buffer chunk N (8000 bytes)` が12回発生**。文脈はファイル転送セッション後のEPUB読書中(グレースケール描画の8KB×6チャンク確保)。**ただしクラッシュせずグレースフルに継続**(ユーザー体感では残像程度)— 旧ベースで `std::bad_alloc` 死していた同種の状況を、このFWはエラーログ+フォールバックで乗り切っている
- `[ERR] [SCT] Deserialization failed: Parameters do not match` は旧FW(crosspoint-reader-cjk)時代の `.crosspoint` セクションキャッシュとの形式不一致による再パース(想定内・無害)。気になる場合はSDの `.crosspoint` を削除してクリーンに
- 残像(ゴースト)所見への対応: 設定「リフレッシュ頻度」(デフォルト15ページ)を短くすることで軽減可能な見込み

## 2-4. upstream状況

- ライセンス: **MIT**
- 説明: 「Xteink X3(/X4)向けの日本語特化ファームウェア」— READMEに「**現状 Xteink X3 でのみ動作確認。X4 での動作は未確認**」と明記
- 活動状況: master最終コミット 2026-05-01。ただし `feature/66-sd-firmware-update` ブランチが **2026-07-13(昨日)まで活発に更新**されており、開発は継続中
- リリース: v0.1.7(2026-04-30)+dev pre-release多数。本家v1.2系ベース(PR #875 X3ハードウェアサポート等を取り込み)
- X4関連issue: 見当たらず(X3関連が中心)。X4対応の予定に関する明示的な言及は未発見
- 時刻処理(ClockSync凍結中のため記録のみ): X3はDS3231 RTCチップ搭載でスリープ画面カレンダーの時刻保持に活用(issue #40)。X4はRTC非搭載・スリープ=完全電源断のため、同機能の可搬性は要実機確認

## 評価用テストデータ

- **縦書きテストEPUB**: `D:\X4\backup\test_book_vertical.epub`(Claude Code生成、5KB)
  - `writing-mode: vertical-rl`(CSS、`-epub-`/`-webkit-`プレフィックス併記)+ `page-progression-direction="rtl"`(spine)
  - 第一章: 句読点・鉤括弧(「」『』())・長音符・小書き仮名・ルビ(`<ruby>`)・漢数字/算用数字・英単語混在・長段落
  - 第二章: 画像(PNG、市松+円)とテーブル → #11の確認を兼ねる

## STEP 3: 移行判断の材料整理(2026-07-14)

- **No-Go条件への該当: なし**(#1起動○ / #5縦書き○ / #12安定性○)
- 主目的(縦書き・フォントサイズ変更・フォントDL)はすべてX4で動作
- 懸念事項(移行後の監視・対応項目としてSTEP 4に織り込む):
  1. **メモリ逼迫の兆候**(MinFree 3.6KB、BWチャンク確保失敗12回)— クラッシュには至らないが余裕は小さい。MemLog移植(STEP 4-2 #2)を最初に行い、逼迫時の挙動を継続観測する根拠になる
  2. 残像(ゴースト)— リフレッシュ頻度設定で運用調整、不足なら改善検討
  3. KOReader同期は未評価(環境なし)
- 総合: **Go 相当**(判断は本人)
- **本人判断(2026-07-14): Go 確定**。crosspoint-jp を新しい開発正本とし、STEP 4(移植)へ進む。メモリ逼迫・残像は監視/調整項目としてSTEP 4に織り込み

## 特記事項

- サブモジュールは `open-x4-epaper/community-sdk`(現行リポジトリの `open-x4-sdk` と同名パス・別リポジトリ由来)。「SDK無変更」原則は新ベースでも適用可能な構成
- 縦書き関連の未マージ修正ブランチ(`fix/vertical-text-mid-flush-continuation`、2026-04-29)がupstreamに存在。評価で縦書きの不具合に当たったら参照
