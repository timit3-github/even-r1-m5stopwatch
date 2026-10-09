# v0.2.1ソース版の書き込み

今回の配布にはビルド済みバイナリを含めていません。
現在動作しているESP-IDF v5.5.1環境を使ってください。

既存プロジェクトに次のファイルを上書きすれば、ボード固有の設定を維持できます。

- src/r1_wire.c
- include/r1_wire.h
- src/main.c
- include/r1_config.h
- CMakeLists.txt

ESP-IDFのターミナルで既存プロジェクトのルートへ移動し、以下を実行します。

```sh
idf.py build
idf.py -p COM5 app-flash
idf.py -p COM5 monitor
```

COM5は実際のポートに変更してください。app-flashはアプリだけを書き込みます。
今回の修正でNVSの消去や既存bootloaderの変更は不要です。
独自にmain.cを変更している場合は、その設定を新しい版に引き継いでください。
起動時の`PROBE_VERSION=0.2.1`を確認してから純正アプリで再試験します。
詳細はUPDATE_v0.2.1_ja.mdを参照してください。

新規にPlatformIOを使う場合は`pio run`、`pio run -t upload`でビルド・書き込みできます。
このプロジェクトの既定値はESP32-S3 / 4 MB / DIO / 40 MHz / PSRAMなし /
ネイティブUSB Serial/JTAGです。現在のボード設定が異なる場合は維持してください。
