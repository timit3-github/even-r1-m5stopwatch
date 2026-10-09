<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# StopWatchハードウェアと設定

## 物理入力

| 端子 | 接続・用途 |
|---|---|
| GPIO1 | 青KEYB、タップ操作。active-low・内部プルアップ |
| GPIO2 | 黄KEYA、短押しdown・長押しup。active-low・内部プルアップ |
| GPIO47 / GPIO48 | 内部I2C SDA / SCL |
| GPIO13 | CST820B割り込み。本実装はポーリングのため使用しない |

物理ボタンは20ms周期で取得し30msデバウンスします。G2ボタンの最初のupは700ms保持で発生し、
以降は500msごとに繰り返します。0に設定すると繰り返しを無効にします。

```c
#define R1_NAV_BUTTON_HOLD_MS 700
#define R1_NAV_BUTTON_REPEAT_MS 500
```

解放で繰り返しと送信待ちの繰り返しを止めます。通信が遅れても過去の回数をまとめて送りません。
通知には125ms以上の最小間隔があるため、その値より速い設定はそのままの速度になりません。
G1/G2の操作はそれぞれ独立して判定します。

## タッチ

CST820B、I2Cアドレス0x15、100kHzで接触状態と座標を読みます。
M5IOE1は0x4F、次に0x6Fを確認し、タッチリセットを10ms Low、50ms Highにします。
M5IOE1_PIN_4はライブラリの列挙値3、レジスタのbit3です。
このピンの出力・上下拉・ドライブ設定と出力値のみ変更し、他のIO拡張端子を保持します。
回路図のタッチ電源はL2です。AMOLEDの電源有効化や画面描画は行いません。
M5Unifiedや上流C++ライブラリはビルド依存物に追加していません。

```c
#define R1_TOUCH_ENABLED 1
#define R1_TOUCH_POLL_MS 20
#define R1_TOUCH_STALE_MS 300
#define R1_TOUCH_SWIPE_PX 60
#define R1_TOUCH_TAP_SLOP_PX 20
#define R1_TOUCH_SWIPE_REVERSE 0
```

スワイプ閾値はタップ許容移動量より大きくしてください。
座標は生値で、表示回転の補正はしません。上下が逆ならREVERSEを1にします。
長押し・二度押し・short→longの時間はR1_BUTTON_HOLD_MS / DOUBLE_MS /
FOLLOWUP_HOLD_MSを共用します。タッチにはGPIOの追加デバウンス時間を入れません。

タッチ取得とバッテリー取得は別タスクで、共有I2Cバスのアクセスは直列化します。
判定とBLE通知はNimBLEホストで行います。タッチ取得失敗、古いデータ、キューあふれ時は
判定を取り消し、離した状態を確認するまで操作を再開しません。
長押しをすでに開始していた場合は終了イベントを送ります。

statusの確認項目:

```text
TOUCH_PANEL enabled=1 valid=1 down=0 x=200 y=200 age_ms=10
```

値は例です。起動時IOE_READYとTOUCH_IDENT、操作時TOUCH_EDGEとTOUCH_GESTUREが出ます。
TOUCH_GESTURE eventsは1=単押し、8=二度押し、2=長押し、16=short→long、4=終了、32=上、64=下。
複合イベントはビットの合計です。TOUCH_INIT_FAILEDやTOUCH_READ_FAILEDが続く場合は
アドレスを伏せた起動ログとstatusを使って調査します。

## バッテリー

PM1アドレス0x6E、VBATレジスタ0x22/0x23のlittle-endian mVを取得します。
初回は取得値をそのまま使い、以後は旧値7:新値1で平滑化します。
3300mV以下0%、4200mV以上100%、中間は直線換算です。
2500mV未満/4500mV超は異常値として採用しません。

```c
#define R1_BATTERY_PM1_ENABLED 1
#define R1_BATTERY_POLL_MS 1000
#define R1_BATTERY_NOTIFY_MS 30000
#define R1_BATTERY_EMPTY_MV 3300
#define R1_BATTERY_FULL_MV 4200
```

取得失敗時は正常値を保持し、初回取得前/機能無効時はR1_BATTERY_PERCENTを使います。
この既定100%は代替値です。statusのvalid=1は一度以上正常に取得したことを示し、
age_msが増え続ける場合は古い値です。充電中・満充電アイコンの判定は未実装です。
電源管理・スリープ・消費電力最適化は実装していません。

## M5Dialへ変更する場合

StopWatch以外のボードでは、そのボードのI2C/GPIOを確認してください。
M5Dialの旧構成に戻す場合の設定は次のとおりです。

```c
#define R1_BUTTON_GPIO 42
#define R1_NAV_BUTTON_GPIO -1
#define R1_ENCODER_A_GPIO 41
#define R1_ENCODER_B_GPIO 40
#define R1_POWER_HOLD_GPIO 46
#define R1_DISPLAY_BACKLIGHT_GPIO 9
#define R1_TOUCH_ENABLED 0
#define R1_BATTERY_PM1_ENABLED 0
```

GPIO46はStopWatchではAMOLEDデータ線です。上のM5Dial設定をStopWatchへ書き込まないでください。
M5Dialエンコーダーは4エッジを1ステップにし、R1_ENCODER_REVERSEで上下反転できます。
現在の主な検証対象はStopWatchです。M5Dialの過去の接続不具合の原因は未確定です。

## 公式参照

- [StopWatch仕様・ピンマップ・回路図](https://docs.m5stack.com/en/core/StopWatch)
- [電源とIO拡張](https://docs.m5stack.com/en/arduino/stopwatch/m5pm1_m5ioe1)
- [公式UserDemo](https://github.com/m5stack/M5StopWatch-UserDemo/tree/6b4aa125288b6fe9dca661f10159f6e1e5ee785c)
- [M5PM1](https://github.com/m5stack/M5PM1/tree/be9a5456c007c333e7ac963f33bfde1ffa5d82ee/src)
- [M5IOE1](https://github.com/m5stack/M5IOE1/tree/1.0.8/src)
