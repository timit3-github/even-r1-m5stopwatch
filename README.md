<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# Even R1 Compatible Remote for M5Stack StopWatch

M5Stack StopWatchを、iPhoneのEvenアプリに登録してEven G2を操作できる
R1互換リモコンとして使うためのESP32-S3ファームウェアです。
純正R1を使わず、iPhoneのEvenアプリで登録し、G2へ操作イベントを送ります。
既定のボードは **M5Stack StopWatch** です。ディスプレイへの描画は行いません。

プロジェクト版は **0.3.0** です。アプリへ通知する互換用文字列
`R1_APP_VERSION=2.3.2.0007` とは別の番号です。

M5Stack StopWatch、ESP-IDF 5.5.1、Evenアプリ2.3.2、G2 2.3.2.14で、
ペアリング・コンソール操作・物理ボタン・タッチ操作の動作が報告されています。
再起動後のG2再接続に数分かかる場合があります。バッテリー残量の実機表示は未確認です。
ファームウェアの全機能や他のアプリ/G2バージョンとの互換性を保証するものではありません。
Even RealitiesおよびM5Stackの公式製品・公式ファームウェアではありません。

## 操作

| 入力 | 操作 |
|---|---|
| 画面の任意の場所をタップ / G1短押し | short tap |
| 二度タップ / G1二度押し | double tap |
| 700ms接触を保持 / G1長押し | long tap |
| 短押し後300ms以内に再接触し200ms保持 / G1短押し→長押し | short→long（メニュー） |
| 上へ60px以上スワイプ | up |
| 下へ60px以上スワイプ | down |
| G2短押し | down |
| G2を700ms保持 | up、その後保持中500msごとに繰り返し |

単押しは二度押し判定のため、解放後約300ms待ちます。
タップは操作領域に関係なく判定し、二度押しの2回の位置が違っても認めます。
20pxを超える移動はタップ候補を取り消し、60px以上かつ横変位以上の上下移動で
1接触につき1回スワイプを送ります。長押し確定後の移動はスワイプにしません。
長押しは指/ボタンを離すと終了イベントを送ります。
起動・再接続時に押したまま/触れたままの場合は、一度離してから操作してください。

## ビルドと書き込み

ESP-IDF **5.5.1** の環境を使います。配布物はソースのみです。

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor
```

COM5は実際のポートへ置き換えます。Linuxでは/dev/ttyACM0などです。
USB Serial/JTAGを使用します。4MB/DIO/40MHz、PSRAMなしの保守的な設定です。
StopWatchの全Flash容量やPSRAMを使う必要はありません。
以前の設定を引き継ぐ場合は[ビルド・接続手順](docs/BUILD.md)を参照してください。

起動したリモコンをEvenアプリでR1として登録します。G2が接続すると操作できます。
`status` や `auto` の入力は起動の必須手順ではありません。

## 設定

[include/r1_config.h](include/r1_config.h)でGPIO、タップ時間、スワイプ閾値を設定します。

```c
#define R1_NAV_BUTTON_REPEAT_MS 500  /* 0で繰り返し無効 */
#define R1_TOUCH_SWIPE_PX 60
#define R1_TOUCH_TAP_SLOP_PX 20
#define R1_TOUCH_SWIPE_REVERSE 0     /* 1で上下を反転 */
```

物理ボタンが不要ならR1_BUTTON_GPIOとR1_NAV_BUTTON_GPIOを-1にできます。
タッチだけでも動作します。タッチの初期化やログは[StopWatch詳細](docs/STOPWATCH.md)を参照してください。
M5Dialで使う場合は同じ文書のボード変更設定を参照してください。

R1_APP_VERSIONはアプリへ通知する互換用文字列です。既定値2.3.2.0007は試験した設定であり、
純正R1のファームウェアを搭載しているという意味ではありません。

通知するバージョンが純正R1の現行ファームウェアより古いと、EvenアプリがR1のアップデートを
求める場合があります。この要求は、[include/r1_config.h](include/r1_config.h)の
`R1_APP_VERSION`を現行版より大きなバージョン番号の文字列に変更し、再ビルドして書き込むことで
回避できます。これはアプリへ通知する版番号の変更であり、純正R1ファームウェアへの更新や
新しい機能への対応を意味しません。プロジェクト版`0.3.0`は変更する必要がありません。

## バッテリー

M5PM1から取得した電圧を、StopWatch公式デモと同じ3300mV=0%、4200mV=100%の
直線換算で推定します。1秒ごとに測定し、%変化時と30秒ごとにアプリ/G2へ通知します。
電圧推定のため、充電中や負荷によって誤差があります。充電状態の判定は未実装です。
取得失敗時は最後の正常値を保持します。初回取得前は固定100%で、実測値ではありません。
`status` のBATTERY valid=1とage_msを確認してください。

## コンソールとテスト

コマンドは入力後 **Enter** で実行します。

| コマンド | 操作 |
|---|---|
| s / d / h / m / r | 単押し / 二度押し / 長押し / short→long / 長押し終了 |
| u / j | up / down |
| status | 接続・購読・バッテリー・タッチ状態 |
| auto | 送信先の手動選択を解除 |
| help | コマンド一覧 |

Linux等のCコンパイラがある環境でホストテストを実行できます。

```sh
sh tests/run_host_tests.sh
```

ホストテストはプロトコル・入力判定の検証で、実機試験を置き換えるものではありません。
検証範囲は[docs/VALIDATION.md](docs/VALIDATION.md)、変更履歴は[CHANGELOG.md](CHANGELOG.md)、
解析根拠は[docs/PROTOCOL.md](docs/PROTOCOL.md)を参照してください。

## ライセンス

新規作成部分は **Boost Software License 1.0 (BSL-1.0)** です。[LICENSE](LICENSE)を参照してください。
MIT参照実装に基づくファイルには元の著作権表示とMIT条件も保持しています。
各ファイルのSPDXヘッダと[NOTICE.md](NOTICE.md)、[LICENSES](LICENSES)が適用範囲を示します。
ESP-IDFなど、外部依存物にはそれぞれのライセンスが適用されます。

## ログの取り扱い

ファームウェアの調査ログにはBLEアドレス、設定ペイロード、接続情報が含まれます。
`tools/capture.py`で収集したログをIssue等へ添付する際は、アドレス・ユーザー設定を伏せてください。
ログ・NVSダンプ・ローカル設定・ビルド生成物は.gitignoreで除外しています。
