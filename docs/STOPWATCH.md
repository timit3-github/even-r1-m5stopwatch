<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# StopWatchハードウェアと設定

## 物理入力

| 端子 | 接続・用途 |
|---|---|
| GPIO1 | 青KEYB、タップ操作。active-low・内部プルアップ |
| GPIO2 | 黄KEYA、短押しdown・長押しup・短押し→長押しdown。active-low・内部プルアップ |
| GPIO47 / GPIO48 | 内部I2C SDA / SCL |
| GPIO13 | CST820B割り込み。本実装はポーリングのため使用しない |

物理ボタンは20ms周期で取得し30msデバウンスします。G2ボタンの最初のupは700ms保持で発生し、
以降は500msごとに繰り返します。0に設定すると繰り返しを無効にします。
短押しを離してから300ms以内に再押下し200ms保持すると、downを送って保持中500msごとに繰り返します。
最初の短押しのdownは、二度押しの判定を待たずに送ります。

```c
#define R1_NAV_BUTTON_HOLD_MS 700
#define R1_NAV_BUTTON_REPEAT_MS 500
#define R1_NAV_BUTTON_DOUBLE_MS 300
#define R1_NAV_BUTTON_FOLLOWUP_HOLD_MS 200
```

解放で繰り返しと送信待ちの繰り返しを止めます。通信が遅れても過去の回数をまとめて送りません。
通知には125ms以上の最小間隔があるため、その値より速い設定はそのままの速度になりません。
同時押しに該当しないG1/G2の操作はそれぞれ独立して判定します。

## タッチの有効・無効

G1/G2を100ms以内に押し、両方を500ms保持するとタッチを切り替えます。
無効化時はバイブ1回、有効化時は2回です。途中でどちらかを離した場合は切り替えません。
同時押し候補を認めた時点で通常のボタン・タッチ入力とリピートを取り消し、
両方のボタンを離すまで個別の操作と再切り替えを抑止します。G2未接続でも切り替えできます。

既定状態はタッチ有効です。NVSの既存名前空間r1testにtouch_onを保存し、再起動後も保持します。
NVSの初期化時に保存済みのbondや設定を自動消去しません。保存は切り替え時だけ行い、
完了時の追加バイブはありません。保存に失敗した場合はその場の設定を維持し、NVS_SAVEを出力します。
電源を切ると保存済みの状態へ戻る可能性があるため、statusのsavedを確認してください。

手動の無効状態ではタッチのI2C取得を停止します。無操作60秒後の操作待機や、
G2側が通知する入力許可とは別の設定です。待機からボタンで復帰しても手動の無効状態を維持します。
有効に切り替えた時点で接触していた指は、一度離してから操作してください。

```c
#define R1_TOUCH_TOGGLE_WINDOW_MS 100
#define R1_TOUCH_TOGGLE_HOLD_MS 500
#define R1_HAPTIC_ENABLED 1
#define R1_HAPTIC_PULSE_MS 100
#define R1_HAPTIC_GAP_MS 120
#define R1_HAPTIC_DUTY_PERCENT 60
```

バイブはM5IOE1のIO9（列挙値8、bit8）、PWMチャンネル0、5000Hzを使います。
パルス処理は別タスクで行い、処理中だけNO_LIGHT_SLEEPロックを保持します。
停止書き込みに失敗した場合はロックを維持して停止を再試行します。
モーターの初期化・駆動に失敗しても、タッチ切り替えとNVS保存は実行します。
起動時の初期化は別タスクで最大3回試行し、失敗後の次の試行まで800ms待ちます。
すべて失敗した場合も待ち行列とタスクを維持し、次の切り替え時に再試行します。待機中は周期的に再試行しません。
INIT_RETRYのstepは失敗した処理、errorはエラー内容です。READYは初期化成功を示します。
FEEDBACK_UNAVAILABLEのreason=no_workerはタスク・待ち行列の作成失敗、queue_fullは待ち行列の満杯を示します。
IO拡張器の共有レジスタは読み取りから書き込み・確認まで排他し、タッチリセット端子の設定を保持します。

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
#define R1_TOUCH_REPEAT_MS 500
#define R1_TOUCH_TAP_SLOP_PX 20
#define R1_TOUCH_SWIPE_REVERSE 0
```

スワイプ閾値はタップ許容移動量より大きくしてください。
判定後も接触を保持すると500msごとにup/downを繰り返します。途中で追加のスワイプをしても
方向は変わりません。指を離すと止まり、次の接触で新しく方向を判定します。
R1_TOUCH_REPEAT_MSを0にすると、従来どおり1接触につき1回のup/downになります。
座標は生値で、表示回転の補正はしません。上下が逆ならREVERSEを1にします。
長押し・二度押し・short→longの時間はR1_BUTTON_HOLD_MS / DOUBLE_MS /
FOLLOWUP_HOLD_MSを共用します。タッチにはGPIOの追加デバウンス時間を入れません。

タッチ取得とバッテリー取得は別タスクで、共有I2Cバスのアクセスは直列化します。
判定とBLE通知はNimBLEホストで行います。タッチ取得失敗、古いデータ、キューあふれ時は
判定を取り消し、離した状態を確認するまで操作を再開しません。
長押しをすでに開始していた場合は終了イベントを送ります。

statusの確認項目:

```text
TOUCH_PANEL enabled=1 user_enabled=1 saved=1 valid=1 down=0 x=200 y=200 age_ms=10
```

値は例です。起動時IOE_READYとTOUCH_IDENT、操作時TOUCH_EDGEとTOUCH_GESTUREが出ます。
enabledはビルド設定、user_enabledは手動設定です。saved=0は保存・読込の失敗を示します。
操作待機中と手動の無効状態ではvalid=0が正常です。切り替え時はTOUCH_USERを出力します。
TOUCH_GESTURE eventsは1=単押し、8=二度押し、2=長押し、16=short→long、4=終了、32=上、64=下。
128はリピートを示し、複合イベントはビットの合計です。TOUCH_INIT_FAILEDやTOUCH_READ_FAILEDが続く場合は
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
v0.3.1では、操作待機中のバッテリー測定周期は30秒、VIN取得周期は1秒です。
省電力動作と確認結果は[POWER.md](POWER.md)を参照してください。

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
#define R1_HAPTIC_ENABLED 0
```

GPIO46はStopWatchではAMOLEDデータ線です。上のM5Dial設定をStopWatchへ書き込まないでください。
M5Dialエンコーダーは4エッジを1ステップにし、R1_ENCODER_REVERSEで上下反転できます。
現在の主な検証対象はStopWatchです。M5Dialの過去の接続不具合の原因は未確定です。

## 追加機能の実機確認

v0.3.2と同じ動作コードの開発版で、同時押しによるタッチ切り替え、NVSの電源OFF/ON後の状態保持、
ボタンとスワイプのリピートを作者実機で確認しました。
バイブは電源OFF/ON後に使えなくなる事例を確認しましたが、初期化の再試行を追加した構成では電源OFF/ON後も動作することを確認しました。
ホストテストとESP-IDF 5.5.1のビルドも成功しています。
書き込み後、次の順で操作を確認してください。電源を切る試験ではNVSを消去しません。

| 操作・条件 | 期待する動作 |
|---|---|
| G1/G2をほぼ同時に500ms保持 | タッチ無効、バイブ1回。G1/G2の通常入力が送られない |
| そのまま数秒保持、片方を離して再押下 | 再切り替えせず、個別入力も送られない |
| 両方を離して再度500ms保持 | タッチ有効、バイブ2回。保存完了時の追加バイブなし |
| 無効にして電源OFF/ON | 無効を保持、ボタン操作は可能。statusはuser_enabled=0、saved=1 |
| タッチ有効・無効それぞれの状態で電源OFF/ONを繰り返し、その後切り替え | 毎回バイブが動作。READYで初期化成功を確認し、失敗時はINIT_RETRYのstepとerrorを確認 |
| 無効にして60秒待機し、ボタンで復帰 | 無効を保持。復帰の最初の通常ボタン操作も判定 |
| G2未接続で同時押し | 切り替えとバイブが動作、再接続しても設定を保持 |
| 接触を保持したままタッチ有効化 | 一度指を離すまで操作せず、過去のタップやスワイプを送らない |
| G2単押し / 通常長押し | down1回 / upと500ms周期のup。解放すると停止 |
| G2短押し→300ms以内に再押下し200ms以上保持 | 最初のdownに続いて、downと500ms周期のdown。upは送られない |
| 上下スワイプ後、接触を保持 | 最初の方向で500ms周期のリピート。解放すると停止 |
| リピート中に反対方向へ指を動かす | 元の方向で継続。離してから逆方向へスワイプすると方向が変わる |
| リピート中に同時押し / G2切断 | リピート停止。復帰・再接続で古いリピートが再生されない |
| G1とタッチの各タップ操作 | 単押し、二度押し、長押し、short→longが従来どおり動作 |

バッテリー駆動では、同時押しが待機からの復帰と切り替えを兼ねることも確認します。
バイブ停止後にr1_hapticのロックがActive=0になることは、USB接続中のpmで確認できます。
追加後の長時間稼働と消費電力は未確認です。

## 公式参照

- [StopWatch仕様・ピンマップ・回路図](https://docs.m5stack.com/en/core/StopWatch)
- [電源とIO拡張](https://docs.m5stack.com/en/arduino/stopwatch/m5pm1_m5ioe1)
- [公式UserDemo](https://github.com/m5stack/M5StopWatch-UserDemo/tree/6b4aa125288b6fe9dca661f10159f6e1e5ee785c)
- [M5PM1](https://github.com/m5stack/M5PM1/tree/be9a5456c007c333e7ac963f33bfde1ffa5d82ee/src)
- [M5IOE1](https://github.com/m5stack/M5IOE1/tree/1.0.8/src)
