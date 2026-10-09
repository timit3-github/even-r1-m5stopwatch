# v0.2.7: G2ボタン保持中のup繰り返し

StopWatchのv0.2.6でG2接続・全操作（短押し→長押しのメニューを含む）の成功が
報告されています。今回の変更はG2ボタン保持中のup繰り返しだけです。
再起動後の接続復帰はまだ未確認で、この版もBLE接続処理を変更していません。

## 操作

G2ボタンを保持すると、押下確定から700msで最初のup、以後500msごとにupです。
離すと停止し、送信待ちの繰り返しも破棄します。短押しは従来どおり解放でdown。
長押し時にdownは送らず、G1の判定も維持します。

```c
// include/r1_config.h
#define R1_NAV_BUTTON_HOLD_MS 700
#define R1_NAV_BUTTON_REPEAT_MS 500
```

REPEAT_MSは単位ms。1000で1秒周期、250で250ms周期、0で従来のupを1回だけに戻ります。
ポーリング周期20ms、通知の最小間隔125msの制限があります。G1操作やBLE処理の
タイミングで多少遅れる場合があります。処理が遅れた回数をまとめて再生しません。
接続・購読・タッチ設定などの操作用リンク条件を失うと停止します。保持したまま
接続が復帰しても再開せず、一度離してから押し直します。

## 更新

v0.2.6から次を更新します。

- CMakeLists.txt
- src/main.c
- src/r1_inputs.c
- include/r1_inputs.h

include/r1_config.hにはR1_NAV_BUTTON_REPEAT_MSを追加し、R1_PROBE_VERSIONを
"0.2.7"へ変更します。既存の設定・sdkconfig・NVS・ペアリングは維持してください。
R1_APP_VERSIONの既定値は2.2.6.0009のままです。
差分はPATCH_v0.2.6_to_v0.2.7.diffで確認できます。

既存のStopWatchで次を実行します。

```sh
idf.py clean
idf.py build
idf.py -p COM5 app-flash
idf.py -p COM5 monitor
```

COM5は実際のポートへ置き換えます。起動時PROBE_VERSION=0.2.7を確認してください。

## 確認

G2ボタンを2秒以上保持して確認します。

```text
NAV_BUTTON action=up repeat=0
TOUCH type=4 v0=1 v1=1 queued=1 ...
... 約500ms後 ...
NAV_BUTTON action=up repeat=1
TOUCH type=4 v0=1 v1=1 queued=1 ...
```

保持中はrepeat=1が続き、離すと止まります。短押しでaction=downを確認します。
ホストの通信・入力テストを実施しました。今回の繰り返し機能の実機動作と
ESP向けビルドはこちらでは未確認です。
