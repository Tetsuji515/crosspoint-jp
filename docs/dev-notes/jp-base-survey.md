# crosspoint-jp コード調査(移植前調査、STEP 4-1)

日付: 2026-07-14
比較元: crosspoint-reader-cjk `phase1-complete`(旧ベース)。旧調査 `home-launcher-survey.md`(A-1〜A-10)と同じ観点で差分を取った。
方法: 両リポジトリの直接diff+コード読解(読み取り専用、実装は未着手)

## 結論サマリ

**両ベースは共通祖先(crosspoint-reader本家)を持ち、フレームワーク層はほぼ同一。** 旧ベースの資産は「新ベースの流儀に合わせて書き直す」というより「小さな差分を埋めれば動く」水準。ランチャー移植の技術的障壁は低い(要否判断は別途、下記)。

## 差分表(旧調査A項目との対応)

| # | 項目 | 差分 | 移植への影響 |
|---|---|---|---|
| A-1 | Activity基底・遷移 | **同一**(`Activity.h` は改行差のみ、`ActivityManager.h` は旧ベース側が追加した `goToBookshelf()` の有無のみ)。jpには `ActivityWithSubactivity.h` が追加であるが既存構造への影響なし | ランチャーの `goHome()` 差し替え+`goToBookshelf()` 追加パターンがそのまま使える |
| A-4 | 描画API | 基本API(`drawText`/`fillRect`/`drawRoundedRect`/`fillRoundedRect(Color)`/`drawLine`/`isDarkMode`)は同一。**jp側の追加**: `drawTextVertical`/`drawTextSideways`(縦書き)、`fillRectDither`、SDフォントの8.8固定小数点スケール、`FontCacheManager`。**jp側に無いもの(旧ベースの独自追加)**: `setPartialUpdateRect()`(部分更新ラッパー)、`displayBufferDarkRedrive()`、tiled grayscale strip系、BiDi対応 | **ランチャーのListOnly部分更新はそのまま移植不可**。SDK層には `EInkDisplay::displayWindow()`(EXPERIMENTAL)が存在するが GfxRenderer/HalDisplay に露出していない。対応: ①fast refresh全画面で済ませる(jpのホームがこの方式で実用)、②旧ベースの `setPartialUpdateRect` 相当の小ラッパーを移植する、の二択 |
| A-5 | ボタン入力 | `ButtonNavigator` **完全同一**。`MappedInputManager` は旧ベース側のBluetoothページターナー対応の有無のみで、`mapLabels()` 含むAPIは同一 | 無修正で流用可 |
| A-6 | 時刻処理(**ClockSync凍結中 — 記録のみ**) | jpは main.cpp で起動時に時刻復元: ①DS3231外部RTC(X3のみ、`halRTC`)→ ②ESP-IDF内部復元(`CONFIG_NEWLIB_TIME_SYSCALL_USE_RTC_HRT`、USB給電時)→ ③無効(エポック付近のまま、`isTimeValid()`=false)。**TZはJST-9固定**(`setenv("TZ","JST-9")`)。NTPはWiFi接続時+KOReader同期時。`SETTINGS.rtcEnabled` あり | 旧ClockSyncのJST固定・WiFiついで同期に相当する仕組みは**jpに既にある**(SDへの時刻永続化のみ無い)。カレンダー表示を移植する場合も `isTimeValid()` を使えば追加実装ほぼ不要。凍結解除の要否判断材料として記録 |
| A-7 | バッテリー | `BaseTheme::drawBatteryRight()` あり(同一系) | 流用可 |
| A-8 | i18n/UIフォント | 仕組みは同系(`tr()`/YAML)。ただしjpのUI日本語表示はSDカードフォントシステム(`SdCardFontSystem`、`.cpfont`)ベースで、旧ベースの「906文字スパース組み込みCJKフォント(UI_20)」は無い | ランチャーはASCII設計なので影響なし |
| A-9/A-10 | スリープ/Quick Resume | 同系(deep sleep=完全リセット、起動時 `goToReader` リダイレクト) | 流用可 |
| — | 組み込みフォント | **フォントID・セットとも同一**(NotoSans/Serif 12/14/16/18、OpenDyslexic、UI_10/12、SMALL=NotoSans8)。`fontIds.h` のID値まで一致 | **カレンダー+カルーセルのフォント設計(18/12/8pt)がそのまま成立** |

## ホーム画面の実装(ランチャー移植の要否再評価の材料)

jpのホームは本家v1.2系の本棚(`HomeActivity`)+jp独自改良:
- 最近の本のカバー表示+読書状態アイコン(著者・タイトル下)
- メニュー: ファイル一覧/最近の本/(OPDS)/**青空文庫**(jp独自!)/ファイル転送/設定
- テーマ3種: `CLASSIC` / `LYRA` / `LYRA_3_COVERS`(設定で切替)

**旧ベースのランチャー(アプリ一覧+月間カレンダー)に相当する画面は無い。**

判断材料:
- **jpホームで足りるもの**: 本棚機能(LIBRARY相当そのもの)、各機能への導線、テーマによる見た目の選択肢
- **jpホームに無いもの**: 月間カレンダー表示、将来アプリ(NOTES/WEATHER/HABITS等)の入口となるアプリレジストリ、HUD調デザイン
- **移植コスト(やる場合)**: 低〜中。`LauncherActivity`(~600行)+`AppRegistry` はフレームワーク同一・フォントID同一のためほぼそのまま移植可能。要対応は3点: ①`setPartialUpdateRect` ラッパー(または全画面fast refreshで代替)、②`getButtonHintInsets`(旧ベース独自ヘルパー、jpのテーマ従来ヘルパーで代替可)、③Back階層(`goToBookshelf()` 追加と `goHome()` 差し替え — 旧ベースで確立済みのパターン)

## スリープ画面・BMPビューア(スライドショーフェーズ要否の材料)

- スリープ画面モード6種: `DARK`/`LIGHT`/`CUSTOM`(SDルートの `/sleep.bmp`)/`COVER`/`BLANK`/`COVER_CUSTOM`
- **`/sleep`(または `/.sleep`)フォルダの複数BMPからランダム選択**が実装済み(直近表示分を除外するローテーション付き、`APP_STATE.recentSleep`)
- **WebUIにスリープ画像の管理機能あり**: アップロード/一覧/サムネイル/削除(`/sleep`、`/api/sleep/*`)
- カレンダーオーバーレイはX3限定(`gpio.deviceIsX3() && SETTINGS.rtcEnabled` ゲート)
- 要件(`images/sleep/`、固定/ランダム/順送り)との対応: **固定=CUSTOM、ランダム=フォルダランダムで既存機能がカバー。「順送り」のみ未実装**
- → スライドショーフェーズは「順送りモードの追加」程度まで**大幅縮小できる見込み**(PC側 `convert_image.py` はベース非依存で有効)

## 移植対象の整理(指示書4-2の優先順位に対応)

| 順 | 資産 | 本調査の結果 |
|---|---|---|
| 1 | デバイス癖・運用ノウハウ | `PORTING-CONTEXT.md` に移設済み(完了) |
| 2 | MemLog | 移植可能・推奨。評価#7でメモリ逼迫の兆候(8KBチャンク確保失敗)を観測済みのため常設価値が高い。`-DENABLE_MEMLOG` フラグ化は移植時に実施 |
| 3 | クラッシュ修正2件 | **移植しない**(評価で再現せず)。なおjpの `GfxRenderer` には旧ベースで入れた「部分更新バッファのフォールバック」に相当する箇所自体が無い(部分更新未使用のため問題も起きない) |
| 4 | フォント生成パイプライン | 当面**移植不要**(評価#3: フォントDL機能がX4で動作、全フォント導入済み)。独自グリフセットが必要になった場合のみ `.cpfont` 形式向けに改修(形式は `FontInstaller.h` の `CPFONT\0` マジック) |
| 5 | ランチャー+AppRegistry | 技術的には移植容易(上記)。**要否は本人判断** |
| — | ClockSync | **移植しない(凍結)**。jpの既存時刻処理で大半が代替可能なことを記録済み(A-6) |
