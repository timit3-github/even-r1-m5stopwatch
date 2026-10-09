# v0.2.5: M5DialをG2の操作器にする

v0.2.3のGPIO取得と、前の操作から時間を空けた長押しの認識はユーザー実機で確認されました。
v0.2.4で二度押しの認識が確認されました。短押し→長押しはtype 1→0では動かず、
v0.2.5で専用type 9へ修正しました。UPDATE_v0.2.5_ja.mdを参照してください。

## 物理操作

| 入力 | G2へ送る操作 | 既存コンソール相当 |
|---|---|---|
| ロータリーの一方向 | type 4、v0=1、v1=1 | u |
| ロータリーの反対方向 | type 5、v0=1、v1=1 | j |
| G42を短く押して離す | 300ms待って単押し、type 1 | s |
| 解放後300ms以内にもう一度短く押す | 二度押し、type 2 | d |
| G42を700ms保持 | 長押し開始、type 0 | h |
| 短押し後300ms以内に再び押して200ms保持 | tap-then-long、type 9を1回 | m / menu |
| 長押し後に離す | 長押し終了、type 8 | r |

回転方向とG2の上下方向が希望と逆ならR1_ENCODER_REVERSEを1にします。
A=G41、B=G40。既定ではAB=(A<<1)|Bが3→1→0→2→3でuを送信し、逆でjを送信します。
方向は実際に回して確認してください。G2のUIによってスワイプの動作は異なります。
公式仕様の16クリック / 64パルスに合わせ、4遷移を1クリックとします。
二度押し判定のため、単押しは解放確定後300ms待ちます。
短押し→長押しでは最初の単押しを送らず、専用type 9へ置き換えます。
300ms / 200msはボタンの分類用の閾値で、物理操作に合わせて調整できます。
コンソールの全コマンドは従来どおりEnterで確定します。

## 書き込み

v0.2.4からの更新はUPDATE_v0.2.5_ja.mdを参照してください。
v0.2.3からは先にUPDATE_v0.2.4_ja.mdのヘッダ定義追加も実施します。
v0.2.2以前の既存プロジェクトでは次も更新・追加します。

- CMakeLists.txt
- src/CMakeLists.txt
- src/main.c
- src/r1_inputs.c（追加）
- include/r1_inputs.h（追加）
- include/r1_config.h

ボード設定、sdkconfig、NVS、BLEの識別子は維持できます。
新しいヘルパーのCMake登録も必要なのでsrc/CMakeLists.txtを忘れないでください。
現在のESP-IDF 5.5.1ターミナルで:

```sh
idf.py clean
idf.py build
idf.py -p COM5 app-flash
idf.py -p COM5 monitor
```

v0.2.2ですでにGATT_LAYOUTが期待値なら、sdkconfigの再変更は不要です。
v0.2.1以前から更新する場合はFLASH_ja.mdのPPCP更新も実施してください。
M5Unified/M5GFX/Arduinoの追加インストールは不要です。
この配布はソース版で、こちらでESP向けビルド・物理GPIO動作は未確認です。

## 初回の確認

1. 起動時にPROBE_VERSION=0.2.5とBOARD_INPUTS button=42 encoder_a=41 encoder_b=40を確認。
2. G2接続後、statusのrole=glasses ch1=1を確認。
3. ゆっくり1クリックずつ回し、ENCODER→TOUCH type=4または5→TX_CH1 len=11を確認。
4. 短押しでBUTTON events=1→TOUCH type=1、保持でevents=2→type=0、
   離すとevents=4→type=8を確認。

回転方向が逆の場合、include/r1_config.hでR1_ENCODER_REVERSEを0から1にして再ビルドします。
回転1クリックあたりの量はR1_ENCODER_EDGES_PER_STEP=4で決まります。

## 入力処理

回転は両相の立ち上がり・立ち下がりをGPIO ISRで採取し、128要素のキューへ送ります。
ISR内ではBLE・ログ・NVS処理を行いません。NimBLEホスト側で20msごとに有限個の
エッジを処理し、正規のAB遷移だけを数えます。往復するチャタリングは差し引き、
不正な同時2bit遷移は途中のカウントを破棄します。キューあふれ時はログを出して
再同期します。ISRはIRAM指定なしなのでflash処理中などの高速回転で取りこぼす可能性はあります。

ボタンは30ms連続した状態変化を確定し、700msの保持は確定した押下から測ります。
短押しを300ms保留し、再押下があれば二度押しまたは短押し→長押しへ分類します。
二度目の保持は200msで長押しへ分類するため、二度押し時はそれより早く離します。
起動時から押されていたボタンは、一度離すまで操作として送りません。

物理ボタンを回転より優先し、通常イベントは直前の操作から128ticks（125ms）以上
空けて送信します。長押し終了type8はG2側の短間隔抑制の例外なので早く送れます。
速い回転は符号付き最大4クリックまで保留します。反対方向は相殺し、上限を超える
回転量は捨てます。通常速度では1クリックが1スワイプです。
未接続・未購読・タッチ無効・選択先不一致の間は保留操作を破棄し、後から再生しません。

## 電源・画面

M5Dial公式の指示に合わせGPIO46をHighにして電源保持します。
GPIO9はLowにしてバックライトを消します。LCDやタッチパネルそのものの初期化・
スリープコマンドは送信していません。低消費電力化は別途の調整が必要です。
USB使用時もこの設定で動作させる構成です。

汎用ESP32-S3に戻す場合は、BUTTON_GPIO、ENCODER_A_GPIO、ENCODER_B_GPIO、
POWER_HOLD_GPIO、DISPLAY_BACKLIGHT_GPIOをそれぞれ-1にします。

公式資料:
- https://docs.m5stack.com/en/core/Dial
- https://github.com/m5stack/M5Dial/blob/master/src/M5Dial.h
- https://docs.m5stack.com/en/arduino/m5unified/m5unified_appendix
- https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/gpio.html
