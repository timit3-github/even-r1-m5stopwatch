# v0.2.9: M5Stack StopWatch

v0.2.6でStopWatchとG2の接続・全操作（short→long含む）の成功が報告されています。
v0.2.7のup繰り返しも成功の報告があります。表示バージョン2.3.2.0007では、
再起動後に数分経ってからG2が再接続した報告があります。遅延原因と安定性は未確定です。
v0.2.8ではPM1から取得した電圧による推定残量%をアプリ/G2へ反映します。
設定と確認手順はUPDATE_v0.2.8_ja.mdを参照してください。
v0.2.9は画面全体のタップと上下スワイプを追加します。UPDATE_v0.2.9_ja.mdを参照してください。
公式ピンマップは青KEYB=G1、黄KEYA=G2です。ユーザー指定のGPIOを優先して割り当てます。

| 入力 | 操作 | 通知 / コンソール相当 |
|---|---|---|
| G1 青: 単押し | tap | type 1 / s |
| G1 青: 二度押し | double tap | type 2 / d |
| G1 青: 単独長押し | long tap | type 0 / h、解放でtype 8 / r |
| G1 青: 短押し→長押し | tap-then-long | type 9 / m、解放でtype 8 / r |
| G2 黄: 短押し | down | type 5、v0=1、v1=1 / j |
| G2 黄: 700ms長押し | 最初のup、その後500msごとにup | type 4、v0=1、v1=1 / u |

G1はv0.2.5の判定を継続します。単押しは二度押し判定のため解放後約300ms待ちます。
短押しの解放後300ms以内に再び押し200ms保持すると専用type 9です。
この専用type 9によるメニュー操作は、StopWatch実機で成功が報告されています。

G2は30msのチャタリング除去後、押下確定から700msを測ります。
短押しのdownは解放確定時に送ります。長押しは700msに達した時点で最初のupを送り、
以後、保持中は500msごとにupを送ります。長押し開始時・解放時にdownを送りません。
解放を検出すると繰り返しを止め、送信待ちの繰り返しupも取り消します。
G2の操作用リンクがなくなると保留操作とボタン状態をリセットします。
そのまま保持したまま接続が戻っても、いったん離して押し直すまで再開しません。
処理が遅れた場合は1回だけupを発生させ、遅れた回数をまとめて送りません。
G2には二度押し判定を入れず、短く2回押すとdownを2回送ります。
閾値はinclude/r1_config.hのR1_NAV_BUTTON_HOLD_MSで変更できます。
繰り返し間隔は同じファイルで次を変更します（単位ms）。

```c
#define R1_NAV_BUTTON_REPEAT_MS 500
```

1000なら1秒ごと、250なら250msごと。0なら繰り返さずupを1回だけ送ります。
判定は20ms周期のポーリングで行い、通知には既存の125ms以上の最小間隔があるため、
設定値より短い周期で送信はしません。G1の操作やBLE処理で通知が遅れる場合があります。
UIによってスワイプの見え方は異なり、down/upの通知形式は従来のj/uと同じです。

## GPIOと周辺機器

G1/G2は入力・内部プルアップ・active-lowとして読みます。
StopWatchのボタンプルアップ電源とESP32-S3はL2に属し、公式資料ではM5PM1起動時に
L1/L2/L3Aが自動的に有効になります。この版はボタンを直接GPIOで取得します。
M5Unified、ディスプレイ、音声のドライバは使用していません。
GPIO47/48の共有I2CでPM1の電圧とCST820Bのタッチ状態を読みます。
M5IOE1のタッチリセットピンだけを設定します。AMOLEDは初期化しません。
深いスリープ、電源管理・消費電力最適化は今回実装していません。

旧M5DialのG40/G41エンコーダー、G46電源保持、G9バックライト制御はすべて無効です。
StopWatchではG46がAMOLEDのデータ線なので、M5Dialの電源保持設定を流用しません。

## 書き込み

同じESP32-S3 / ESP-IDF 5.5.1を使えます。PSRAMは使用せず、配布のFlash設定は
従来の保守的な4MB / DIO / 40MHzのままです。公式StopWatchの物理Flashは16MBですが、
今回のアプリは全容量を使う必要がありません。

v0.2.8のプロジェクトから次を更新します。

- CMakeLists.txt
- src/main.c
- src/CMakeLists.txt
- src/r1_battery.c（共有I2Cへ変更）
- src/r1_i2c.c、src/r1_touch.c、src/r1_touch_gesture.c（追加）
- include/r1_i2c.h、include/r1_touch.h（追加）
- include/r1_config.h（タッチ設定追加、PROBE_VERSIONを0.2.9へ）

SDKやGATT設定は維持します。StopWatchへの初回書き込みは、新しいプロジェクトの
bootloaderとpartition tableも合わせるためapp-flashではなくflashを使用します。

```sh
idf.py clean
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor
```

COM5はStopWatchの実際のポートへ置き換えます。
必要なら公式手順でダウンロードモードへ入ります。USBでPCに接続し、電源ボタンを
約2秒保持して緑LEDが点灯したら離します。G1/G2とは別の電源ボタンです。

StopWatchはM5DialとBLE MACが異なるため、新しい機器としてEvenアプリで登録します。
旧M5Dialの電源を切ってから接続を確認してください。旧ボードのbondをコピーしません。
status / autoは操作開始の必須コマンドではありません。

## 初回の確認

```text
PROBE_VERSION=0.2.7
BOARD_INPUTS button=1 nav_button=2 encoder_a=-1 encoder_b=-1 ...
IDENTITY ... app=2.2.6.0009 ...
```

G2へ送信できる状態はstatusでrole=glasses ch1=1です。
role=phone ch1=0 ch2=1しか出ない場合はiPhoneだけの接続なので、G2操作を試す前に
CONNECTED / SUBSCRIBE / DISCONNECTEDのログでG2側の接続状態を調べます。

GPIOは未接続時でもBUTTON_RAW / NAV_BUTTON_RAWで確認できます。ready=0なら
G2操作の受付条件が満たされていません。未接続時の押下を接続後に再生しません。
起動時や接続復帰時に押されていたボタンは、一度離してから使います。

G2短押しの期待ログ:

```text
NAV_BUTTON action=down
TOUCH type=5 v0=1 v1=1 queued=1 ...
TX_CH1 ... len=11
```

G2長押しの期待ログ（最初はrepeat=0、繰り返しはrepeat=1）:

```text
NAV_BUTTON action=up repeat=0
TOUCH type=4 v0=1 v1=1 queued=1 ...
TX_CH1 ... len=11
... 約500ms後 ...
NAV_BUTTON action=up repeat=1
TOUCH type=4 v0=1 v1=1 queued=1 ...
TX_CH1 ... len=11
```

v0.2.6の全操作は実機確認済みです。今回の繰り返し機能はまだ実機未確認です。
ホストテストで500ms反復、間隔変更・無効化、解放と接点揺れで停止、
処理遅延時の一括再生なし、接続喪失時の状態リセット、時刻の桁あふれを検証しました。
こちらではESP向けビルドを実施していません。

## 表示バージョンと接続調査

R1_APP_VERSIONは2.2.6.0009へ戻しました。2.3.2.9999で警告が消えたことは実機確認済み
ですが、今回のG2未接続との因果関係はまだ未確認です。
表示バージョンだけの影響を切り分けるには、まず同じM5Dialのv0.2.5でこの値だけを
戻して比較してください。StopWatchへの移行ではMACとボードも変わります。
同じR1_APP_VERSION値に戻して接続が改善したか、同じボードのログで確認します。

## M5Dialへ戻す場合

include/r1_config.hを次へ変更します。

```c
#define R1_BUTTON_GPIO 42
#define R1_NAV_BUTTON_GPIO -1
#define R1_ENCODER_A_GPIO 41
#define R1_ENCODER_B_GPIO 40
#define R1_POWER_HOLD_GPIO 46
#define R1_DISPLAY_BACKLIGHT_GPIO 9
```

公式資料:
- https://docs.m5stack.com/ja/core/StopWatch
- https://docs.m5stack.com/ja/arduino/stopwatch/m5pm1_m5ioe1
