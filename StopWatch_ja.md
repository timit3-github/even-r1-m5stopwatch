# v0.2.6: M5Stack StopWatch

既定の物理入力をM5DialからStopWatchへ変更しました。
公式ピンマップは青KEYB=G1、黄KEYA=G2です。ユーザー指定のGPIOを優先して割り当てます。

| 入力 | 操作 | 通知 / コンソール相当 |
|---|---|---|
| G1 青: 単押し | tap | type 1 / s |
| G1 青: 二度押し | double tap | type 2 / d |
| G1 青: 単独長押し | long tap | type 0 / h、解放でtype 8 / r |
| G1 青: 短押し→長押し | tap-then-long | type 9 / m、解放でtype 8 / r |
| G2 黄: 短押し | down | type 5、v0=1、v1=1 / j |
| G2 黄: 700ms長押し | upを1回 | type 4、v0=1、v1=1 / u |

G1はv0.2.5の判定を継続します。単押しは二度押し判定のため解放後約300ms待ちます。
短押しの解放後300ms以内に再び押し200ms保持すると専用type 9です。
このメニュー通知のG2 2.3.2.14での成功はまだ未確認です。

G2は30msのチャタリング除去後、押下確定から700msを測ります。
短押しのdownは解放確定時に送ります。長押しは700msに達した時点でupを1回送り、
長押し開始時・解放時にdownを送りません。保持中の繰り返し送信もありません。
G2には二度押し判定を入れず、短く2回押すとdownを2回送ります。
閾値はinclude/r1_config.hのR1_NAV_BUTTON_HOLD_MSで変更できます。
UIによってスワイプの見え方は異なり、down/upの通知形式は従来のj/uと同じです。

## GPIOと周辺機器

G1/G2は入力・内部プルアップ・active-lowとして読みます。
StopWatchのボタンプルアップ電源とESP32-S3はL2に属し、公式資料ではM5PM1起動時に
L1/L2/L3Aが自動的に有効になります。この版はボタンを直接GPIOで取得します。
M5Unified、ディスプレイ、タッチ、音声、PM1/IO拡張のドライバを追加していません。
深いスリープ、電源管理・消費電力最適化は今回実装していません。

旧M5DialのG40/G41エンコーダー、G46電源保持、G9バックライト制御はすべて無効です。
StopWatchではG46がAMOLEDのデータ線なので、M5Dialの電源保持設定を流用しません。

## 書き込み

同じESP32-S3 / ESP-IDF 5.5.1を使えます。PSRAMは使用せず、配布のFlash設定は
従来の保守的な4MB / DIO / 40MHzのままです。公式StopWatchの物理Flashは16MBですが、
今回のアプリは全容量を使う必要がありません。

v0.2.5のプロジェクトから次を更新します。

- CMakeLists.txt
- src/main.c
- src/r1_inputs.c
- include/r1_inputs.h
- include/r1_config.h（今回のボード変更を反映）

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
PROBE_VERSION=0.2.6
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

G2長押しの期待ログ:

```text
NAV_BUTTON action=up
TOUCH type=4 v0=1 v1=1 queued=1 ...
TX_CH1 ... len=11
```

実機未確認です。既存プロトコル、短押しdown・長押しupのみ・解放で追加通知なし・
700ms境界・短押しの連続・2ボタンの状態独立についてホストテストを実施しています。

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
