# v0.2.9: StopWatchのタッチ操作

画面全体をタップ入力に使い、上下移動でup/downを入力できます。
画面上の操作領域やボタン表示はありません。物理G1/G2とコンソールも引き続き使えます。

| タッチ操作 | G2へ送る操作 |
|---|---|
| 短く触れて離す | short tap (s/type1) |
| 離してから300ms以内にもう一度短く触れる | double tap (d/type2) |
| 700ms触れ続ける | long tap (h/type0)、離すと終了(r/type8) |
| 短押し後300ms以内に再度触れ、200ms保持 | short→long (m/type9)、離すと終了(r/type8) |
| 指を上へ60px以上移動 | up (u/type4) |
| 指を下へ60px以上移動 | down (j/type5) |

タップは座標の領域に関係なく接触状態で判定し、二度押しの2回の場所が違っても認めます。
単押しは二度押し判定のため、離してから300ms待ちます。
スワイプは接触開始点からの上下変位が60px以上、かつ横変位以上になった時点で送ります。
1回の接触で送るスワイプは1回です。次のスワイプは一度離してから行います。
表示回転は設定せずコントローラーの生座標を使います。方向が逆ならREVERSEを1にしてください。

20pxを超える移動はタップ・長押し候補を取り消します。60pxに達しない移動や横スワイプは
タップにも上下操作にもなりません。長押し確定後の移動はスワイプにしません。
読み取り失敗・300ms以上古いデータ・キューあふれ・操作リンクの喪失時は判定をリセットし、
離した状態を確認するまで次の接触を操作にしません。通信失敗をタップや長押しとして扱いません。
すでに長押し開始を送っていた場合、読み取り異常時は終了イベントを送ります。
起動・再接続時に触れていた指も、離してから操作してください。

## 設定

include/r1_config.h:

```c
#define R1_TOUCH_ENABLED 1
#define R1_TOUCH_POLL_MS 20
#define R1_TOUCH_STALE_MS 300
#define R1_TOUCH_SWIPE_PX 60
#define R1_TOUCH_TAP_SLOP_PX 20
#define R1_TOUCH_SWIPE_REVERSE 0
```

スワイプの必要移動量はR1_TOUCH_SWIPE_PXを変更します。タップ許容移動量より大きくしてください。
長押し・二度押し・短押し→長押しの時間は既存のR1_BUTTON_HOLD_MS / DOUBLE_MS /
FOLLOWUP_HOLD_MSと共通です。物理ボタンを使わない場合はR1_BUTTON_GPIOと
R1_NAV_BUTTON_GPIOを-1にできます。タッチだけでも通知キューは動きます。
M5Dialなど別ボードではR1_TOUCH_ENABLEDとR1_BATTERY_PM1_ENABLEDを0にしてください。

## 初期化

StopWatchのCST820B(0x15)を20ms周期でポーリングします。GPIO13の割り込みは使用しません。
接触数・座標・Down/Up/Contactイベントを読むため、表示やM5Unifiedの初期化は不要です。
M5IOE1を0x4F/0x6F、100k/400kHzで確認し、公式手順に合わせタッチリセットを
10ms Low、50ms Highにします。M5IOE1_PIN_4はライブラリの値3、レジスタのbit3です。
このビットの出力・上下拉・ドライブ設定と出力値だけ変更し、他のIOEピンを保持します。
公式回路図ではタッチ電源はL2です。AMOLED電源L3Bの有効化や画面の初期化は行いません。

バッテリーとタッチはGPIO47(SDA)/48(SCL)のI2Cバスを共有します。
バスは起動時に1回作成し、トランザクションはミューテックスで直列化します。
タッチ読み取りは別タスク、判定とBLE通知は従来のNimBLEホストで行います。
バッテリー残量報告、バージョン表示2.3.2.0007、GATT配置とペアリングの処理は継続します。

## 更新・実機確認

ソース一式を更新してください。差分はPATCH_v0.2.8_to_v0.2.9.diffです。
新しいr1_i2c.c/.h、r1_touch.c/.h、r1_touch_gesture.cとsrc/CMakeLists.txtも必要です。
NVSの消去やペアリング解除は不要です。

```sh
idf.py clean
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor
```

COM5は実際のポートへ置き換えます。

1. 起動ログにPROBE_VERSION=0.2.9、IOE_READY、TOUCH_IDENTが出ることを確認。
2. G2の操作用接続が確立してからタッチを試します。再接続には数分かかる場合があります。
3. statusのTOUCH_PANEL valid=1、age_msが小さい値であることを確認。
4. 指を離し、画面中央と周辺でタップ。TOUCH_EDGEとTOUCH_GESTUREが出ることを確認。
5. 上下スワイプ、二度押し、長押し、短押し→長押しを試します。
6. バッテリーのBATTERY valid=1も確認し、共有バスでの残量読み取りを確認。

TOUCH_GESTURE eventsは1=単押し、8=二度押し、2=長押し、16=短押し→長押し、
4=長押し終了、32=上、64=下です。複合値はビットの合計です。
TOUCH_INIT_FAILED / TOUCH_READ_FAILEDが出る場合は、起動からstatusまでのログを保存してください。

## 検証範囲と参照

既存wire/legacy/input/batteryテストと、新規タッチテストをホストで通過しました。
タッチテストは座標復号、位置の違う二度押し、長押し・専用メニュー、移動閾値、
横移動の除外、二重送信防止、起動中接触の除外、時刻カウンターの折返しを確認します。
この環境にはESP32-S3用クロスツールチェーンがなく、IDFビルドと実機確認は未実施です。

- [StopWatch仕様・ピンマップ・回路図](https://docs.m5stack.com/en/core/StopWatch)
- [公式タッチドライバ](https://github.com/m5stack/M5StopWatch-UserDemo/tree/6b4aa125288b6fe9dca661f10159f6e1e5ee785c/main/hal/drivers/cst820)
- [公式IOE初期化とタッチリセット](https://github.com/m5stack/M5StopWatch-UserDemo/blob/6b4aa125288b6fe9dca661f10159f6e1e5ee785c/main/hal/hal_ioe.cpp)
- [M5IOE1 1.0.8レジスタ・ピン定義](https://github.com/m5stack/M5IOE1/tree/1.0.8/src)
