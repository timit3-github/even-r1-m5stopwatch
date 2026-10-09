# ソース版の書き込み

現在のStopWatch版はStopWatch_ja.mdの手順を参照してください。
v0.2.7からv0.2.8への更新はUPDATE_v0.2.8_ja.mdを参照してください。
新しいStopWatchへの初回書き込みはbootloader/partition tableも含むflashを使用します。
以下は既存M5Dialプロジェクトの更新と、旧版からのGATT設定移行の説明です。

v0.2.2からはM5Dial_ja.mdの手順で更新してください。既存sdkconfigの再変更は不要です。
以下はv0.2.1以前から更新するときのGATT設定手順です。

## GATT設定を含むソース版の書き込み

ビルド済みバイナリは含みません。現在のESP-IDF v5.5.1環境を使用してください。
既存プロジェクトへsrc/main.c、include/r1_config.h、CMakeLists.txt、
sdkconfig.defaults、tools/configure_gatt_layout.pyを更新します。
v0.2.1より前から更新する場合はsrc/r1_wire.cとinclude/r1_wire.hも更新してください。
ボード固有の設定や配線変更は維持してください。

既存sdkconfigにはsdkconfig.defaultsの変更が自動反映されません。
プロジェクトのルートで、ビルド前に次を実行してください。

```sh
python tools/configure_gatt_layout.py sdkconfig
idf.py reconfigure
idf.py build
idf.py -p COM5 app-flash
idf.py -p COM5 monitor
```

スクリプトはPPCPの4値だけを0にし、元の設定をsdkconfig.before-r1-v022へ保存します。
COM5は実際のポートへ変更してください。app-flashはアプリのみを書き込みます。
NVS全消去やbootloaderの変更は不要です。
起動時にPROBE_VERSION=0.2.2とGATT_LAYOUTの値を確認します。
再接続・キャッシュの手順はUPDATE_v0.2.2_ja.mdを参照してください。

PlatformIOで既存のsdkconfig.esp32s3を使う場合は、次の手順です。

```sh
python tools/configure_gatt_layout.py sdkconfig.esp32s3
pio run
pio run -t upload
```

新規ビルドではsdkconfig.defaultsを使います。既定値はESP32-S3 / 4 MB /
DIO / 40 MHz / PSRAMなし / ネイティブUSB Serial/JTAGです。
