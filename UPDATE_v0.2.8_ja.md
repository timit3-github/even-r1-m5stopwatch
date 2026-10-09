# v0.2.8: StopWatchのバッテリー残量

M5PM1のI2Cアドレス0x6E、VBATレジスタ0x22/0x23から電圧(mV)を読み、
R1の残量としてEvenアプリのdeviceStatusとG2の旧形式0x8Bに反映します。
StopWatch内蔵バスはSDA=GPIO47、SCL=GPIO48です。M5Unifiedは不要です。
電源・充電器・PM1のGPIO・ウォッチドッグの設定レジスタには書き込みません。
読み取りは別タスクで行い、BLE通知は従来どおりNimBLEホストで行います。

## 残量の意味と更新

StopWatch公式UserDemoと同じ、3300mV=0%、4200mV=100%の直線換算です。
初回はそのまま採用し、以降は前回7:新規1の電圧平滑化を行います。
精密な電池残量計ではありません。充電中・負荷・電池の状態によって誤差があります。
充電中/満充電アイコンの判定は未実装で、既存の「非充電」値を維持します。

1秒ごとに測定し、%が変わったときと30秒ごとに購読済みの接続へ通知します。
初回接続時の状態通知と、アプリからの状態取得にも最新値を使います。
アプリ画面の表示更新時期はアプリ側の処理によります。
取得失敗時は前回の正常値を保持します。まだ一度も読めていない場合は従来の固定100%を使い、
ログに失敗を表示します。これは実測100%を意味しません。
2500mV未満/4500mV超の値は異常値として採用しません。
PM1の睡眠復帰のためSTART/addressで起こし、失敗時はホストの100kHz/400kHzを切り替えて再試行します。

`status` の例（値は例示）:

```text
BATTERY valid=1 percent=50 filtered_mv=3750 age_ms=200
```

valid=1は正常値を少なくとも一度取得した状態です。age_msが増え続ける場合は読み取りが止まっています。
valid=0、age_ms=-1は未取得です。成功時はR1BATのBATTERYログも出ます。
充電ケーブルを抜いた状態で電圧・%・アプリ表示を比較してください。

## 設定

include/r1_config.h:

```c
#define R1_BATTERY_PM1_ENABLED 1
#define R1_BATTERY_SDA_GPIO 47
#define R1_BATTERY_SCL_GPIO 48
#define R1_BATTERY_POLL_MS 1000
#define R1_BATTERY_NOTIFY_MS 30000
#define R1_BATTERY_EMPTY_MV 3300
#define R1_BATTERY_FULL_MV 4200
```

M5DialなどPM1のないボードではR1_BATTERY_PM1_ENABLEDを0にしてください。
実測未取得/無効時の固定値はR1_BATTERY_PERCENTです。
アプリへ返すバージョンはユーザー現在の設定に合わせ2.3.2.0007へ変更しています。
v0.2.7のup繰り返しは実機成功の報告があり、同じ操作処理を維持します。
このバージョン表示でStopWatch再起動後、数分後にG2へ再接続した報告があります。
再接続の遅延原因と安定性はまだ未確定です。

## 更新と確認

同梱ソース一式に更新し、ご自身で変更している設定はr1_config.hへ反映してください。
差分はPATCH_v0.2.7_to_v0.2.8.diffです。新しいCソースとヘッダ、src/CMakeLists.txtも必要です。
NVSの消去やペアリング解除は不要です。

```sh
idf.py clean
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor
```

COM5は実際のポートへ置き換えます。起動時PROBE_VERSION=0.2.8とBATTERYログを確認し、
statusのvalid=1とアプリのR1残量を比較します。読み取り/表示の実機確認は未実施です。
ホストで既存wire/legacy/inputテストと新規電圧換算・異常値保持テストを確認しています。
この環境にはESP-IDFのクロスツールチェーンがなく、ESP32-S3ビルドは未実施です。

## 公式参照

- [StopWatchピンマップ](https://docs.m5stack.com/ja/core/StopWatch)
- [M5PM1ドライバ](https://github.com/m5stack/M5PM1/tree/be9a5456c007c333e7ac963f33bfde1ffa5d82ee/src): readVbat、_readReg16、I2C wake/retry。
- [StopWatch公式残量換算](https://github.com/m5stack/M5StopWatch-UserDemo/blob/6b4aa125288b6fe9dca661f10159f6e1e5ee785c/main/hal/hal_pmic.cpp): battery_millivolts_to_percent、update_bat_level_from_mv。
- [ESP-IDF 5.5.1 I2C API](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/i2c.html)
