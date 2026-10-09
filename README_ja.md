# ESP32-S3 R1互換プロトコル試作 v0.2.1

対象は、純正R1を持たず、iPhoneの純正EvenアプリとG2で接続・操作を調べる環境です。
アプリ v2.3.2 / G2 v2.3.2.14 での実機成功はまだ確認していません。
公開解析の基準は主にG2 2.2.6.10 / R1 2.2.6.0009です。
この版は、旧版の既知の手順を実装し、現行版の要求をUSBログで観測するためのものです。

## v0.2からの修正

実機ログの最初のpairAuth要求はモデル全体のCRC16/MODBUSを使用していました。
v0.2は受信モデルにcompact CCITTのみを認めていたため、この要求を拒否しました。
v0.2.1は両形式のCRCを検証して受け付け、採用した形式を`MODEL_CHECKSUM`で表示します。
CRC検証を省略する修正ではありません。送信モデルのMODBUS形式は従来どおりです。
実際の18バイトを固定テストに追加し、pairAuthとして読めることを確認しました。
登録完了・暗号化・G2操作の実機確認は次の試験で行います。
更新方法と期待するログは`UPDATE_v0.2.1_ja.md`を参照してください。

## v0.1からv0.2の変更

- G2の旧形式コマンド `0x88` をメガネ役割の内部イベントとして処理。
  値 `2` は無線通知そのものではないため、架空の `0x88` 応答は送信しません。
  代わりに旧R1で追跡できたバッテリー `0x8B` と装着状態 `0x8C` を通知します。
- `0x85` タッチ設定、`0x8A` 長押し時間、`0x89` 状態、`0x94` 接続維持に応答。
  既知のSET要求のみを受け付け、受信フレームのbyte 4を1に変更して
  旧R1の長さ（7 / 7 / 7 / 5バイト）で返します。GETや異なる形式はログのみです。
- 操作通知をスマホ向け3バイト形式から、G2向け11バイト `0x61` 形式へ変更。
- 電話役割1つ、メガネ役割1つを管理。役割の重複・上書き・交差を拒否。
  旧R1に合わせ、channel 1のCCCDイベントでもメガネ役割を設定します。
  BLE接続枠は旧R1に合わせて最大3つですが、3つの接続に操作を一斉送信する設計ではありません。
- 電話pairAuthの成功通知を暗号化完了後に送信。
- advStartの接続先12バイトを保存・復元し、接続相手との照合結果を表示。
- 電話とメガネの両役割が埋まると広告停止、切断後は広告再開。
- 設定GET/SETを旧R1のstatus bit 1で区別。
- 相手のMTU交換への応答、再構成タイムアウト、送信期限、接続世代による古い送信の破棄。
- `status` / `event` / `select` / `send` の調査コマンドを追加。

## 継続して実装されている部分

- GAP名 `EVEN R1_XXXXXX`、appearance 0x0240。
- 広告：flags/name/appearance。scan response：company 0x5245、自身のアドレス、15文字の試作シリアル。
- BAE80001サービス、BAE80010/12 Write Without Response、BAE80011/13 NotifyとCCCD。
- EUS断片再構成、非反転CRC-32C、受信compact CCITT / full-model MODBUS、送信MODBUS。
- deviceStatus、deviceInfo、deviceSerial、wearStatus、phone pairAuth。
- userInfo、systemTime、touchSwitch、advStart、healthSettings / systemSettings。
  設定の保存・読み出しだけを行い、健康計測や電源回路の動作は再現しません。
- Just Works bondingとNVSへのbond保存。再接続ごとにpairAuthを行う構成。
- 任意GPIOの単押し・二度押し・700 ms長押し・長押し終了。

## まだ再現していない部分・実機で調べる部分

- 純正アプリの現行登録フロー、シリアル検証、アカウント紐付け。
- 新版で追加・変更されたコマンドや応答。
- directed advertising、正常時の広告速度切替、低消費電力化。
  この版は不足役割がある間、100 msのundirected広告を継続します。
- 接続先のアドレス順序・種類。advStartには種類が含まれず、byte順も未確定です。
  初期設定では一致しなくてもログだけにします。反転を勝手に適用しません。
  `R1_STRICT_TARGET_MATCH=1` で、役割を割り当てる前に厳密な不一致を拒否できます。
  ただしこの設定は旧R1の遅延切断処理まで再現するものではありません。
- `0x8A` の値は旧R1が読むbig-endianでログ表示します。
  公開G2送信処理のlittle-endianとの不一致があり、GPIOの長押し時間は700 msを維持します。
- スワイプの実際の方向・移動量の意味、UIごとのイベント対応。
- DFUサービス・純正OTA、アルゴリズムライセンス、健康計測・履歴。
  未対応要求を成功として返しません。これらの要求で登録が停止する可能性があります。
- EUS再構成は連続した降順と同一CRCを要求します。旧stockの重複断片の置き換えは再現していません。
- 表示するバッテリー100%・非充電・装着中は試作の固定値です。センサー測定値ではありません。
- シリアル `ESP32S3R1TST001` は15文字の試作識別子です。
  deviceInfoの旧版値はプロトコル再現用で、ESPの実際の版は起動時の `PROBE_VERSION=0.2.1` です。

## 書き込み

このv0.2.1配布はソース版です。旧v0.2のバイナリは含めていません。
現在お使いのESP-IDF環境での更新方法は `FLASH_ja.md` を参照してください。
PlatformIOでビルドする場合は以下の方法を使います。

PCにVS Code + PlatformIO、またはPlatformIO CLIを用意します。
iPhone用アプリ開発環境・Xcode・Apple Developer契約は不要です。
プロジェクトは汎用ESP32-S3 DevKitC-1 / ESP-IDF 5.5を対象にしています。
flashは4 MB・DIO・40 MHzとして使用し、PSRAMは使いません。
物理flashが4 MB以上の基板を想定します。基板によってUSB・GPIOなどの調整が必要です。

```sh
pio run
pio run -t upload
pio device monitor -b 115200
```

ESP-IDFを直接使用する場合：

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

`PORT` は実際のCOMポート等に置き換えてください。
初期設定のログ入出力はESP32-S3のネイティブUSB Serial/JTAGです。
DevKitにUSB端子が2つある場合は、ネイティブUSB側を使用してください。
USB-UART側を使用する場合はsdkconfigのコンソールをUARTへ変更してください。
初回はボタンを配線せず、シリアルだけで接続を検証できます。

## 最初の実機試験

1. USBログを開始し、基板を再起動。`IDENTITY` と `ADVERTISING ... rc=0` を記録。
2. G2を通常どおりiPhoneのEvenアプリに接続。
3. アプリのデバイス追加からR1を検索し、`EVEN R1_XXXXXX` を選択。
4. iOSがペアリング確認を出した場合は承認。アプリの結果・エラー文を記録。
5. 接続が進んだら、モニタへ `status` を入力してEnter。
6. `role=glasses ch1=1` が確認できたら、G2の標準画面を表示し、`s`、`d` を
   1秒以上の間隔で試す。表示の変化と入力文字を記録。
7. 次に `h`、`r`、`u`、`j` を試す。`h` は長押しイベント、`r` はその終了イベント。
8. 成功したら電源を入れ直し、再接続と操作を確認する。

iPhoneの設定画面から先に手動ペアリングする必要はありません。
登録に失敗しても、設定やbondをすぐ消さず、起動から失敗までのログを保存してください。
現行版の要求を読み取って、未対応処理を追加するために使います。

| ログ | 意味 |
|---|---|
| `CONNECTED` / `SUBSCRIBE` | BLE接続・通知購読が進んだ |
| `PAIR_PHONE` / `SECURITY ... encrypted=1` | 電話役割設定・暗号化が進んだ |
| `PHONE_AUTH_NOTIFY` | 暗号化後の電話認証通知を送信キューに登録した |
| `ADVSTART_PEERS` | アプリからメガネの接続先2個を受信した |
| `G2_PAIR_ROLE_EVENT` | G2向け旧式役割設定要求を受信した |
| `CCCD_GLASSES_ROLE_EVENT` | channel 1の通知購読イベントからメガネ役割を設定した |
| `G2_TOUCH ... enabled=1` | G2がタッチ有効を要求した |
| `LEGACY_UNIMPLEMENTED` / `UNIMPLEMENTED_SYSTEM_SUBCOMMAND` | 次に解析する未対応要求 |
| `PHONE_ROLE_REQUIRED` | 電話pairAuth前、または異なる接続からEUS要求が来た |
| `ROLE_OCCUPIED` / `ROLE_CONFLICT` | 別の役割所有者、または同じ接続の役割混在 |
| `TOUCH ... queued=0` | メガネ未接続・未購読、タッチ無効、または選択先不一致 |
| `TX_CH1` / `TX_CH2` | 通知をBLEスタックへ渡した。相手が受理した証明ではない |
| `TX_TIMEOUT` / `TX_FAILED` | MTU・送信キュー等のため送信できなかった |

## シリアル調査コマンド

すべてEnterで確定します。

| コマンド | 内容 |
|---|---|
| `help` または `?` | コマンド一覧 |
| `status` | 役割、CCCD、MTU、暗号化、タッチ設定、接続先照合、キュー残数 |
| `s` / `d` | type 1 / 2：タップ / ダブルタップの旧形式イベント |
| `h` / `r` | type 0 / 8：長押し / 終了イベント |
| `u` / `j` | type 4 / 5、付随値1・1。上下の割り当ては実機確認が必要 |
| `event 4 3 1` | type 4、付随値3・1の通知。typeは0,1,2,4,5,8のみ |
| `select 1` | 接続ハンドル1のメガネを操作先に指定 |
| `auto` | 接続中のメガネ役割を操作先にする |
| `send 1 1 0009610001000078563412` | ハンドル1、channel 1へ指定バイトを直接通知 |

`send` は自動の操作ゲートを通らない調査用コマンドです。
接続ハンドルは `status` の値を使ってください。channel 2も指定可能ですが、
このコマンドはCRCやEUSヘッダを自動生成せず、入力したバイトをそのまま送ります。
通常の操作試験には `s` 等を使用してください。

11バイト操作通知は `00 09 61 00 type v0 v1 tick:u32LE` です。
時刻は旧R1の1024 Hz RTOS tickに合わせています。100 tick以内の続けた入力は
旧G2で抑制される可能性があるため警告を表示します。

## ログをファイルに保存する場合

PlatformIOのモニタと同時に同じポートを開かないでください。
付属の収集スクリプトでも、画面表示・ファイル保存・コマンド入力ができます。

```sh
python -m pip install pyserial
python tools/capture.py --port COM5 --output r1_first_test.log
```

Linuxでは `--port /dev/ttyACM0` などを指定。終了はCtrl+C。
ログ、アプリの結果・エラー文、G2画面の反応を一緒に共有してください。
ログには機器アドレス・設定データが含まれるため、公開投稿では伏せてください。

## GPIOボタン

`include/r1_config.h` の `R1_BUTTON_GPIO` を空きGPIOへ変更し、GNDとの間に
スイッチを接続。内蔵プルアップを使います。
USB・BOOT・flashなどに割り当てられているピンは避けてください。
初期設定は `-1`（ボタンなし）です。

## 検証と資料

```sh
sh tests/run_host_tests.sh
```

Windows等では同スクリプト内のCコンパイルコマンドを利用できます。
固定パケット、CRC、断片境界、既知要求の応答、短い要求の拒否、接続先照合を確認します。
ビルドと実機検証の実施状況は `VALIDATION.md`、解析根拠は `PROTOCOL_NOTES.md` を参照。
