# ビルド済みv0.2の書き込み

ESP32-S3、物理flash 4 MB以上、ネイティブUSB Serial/JTAGを使う基板向けです。
PSRAMは不要です。元のESP32用ではありません。
GPIOボタンは無効なので、最初は配線せずシリアルで操作できます。
このビルドの起動・無線動作は実機では未確認です。

この方法はPCのPythonとesptoolだけで書き込めます。
iPhoneアプリの開発環境・ESP-IDFのインストールは不要です。

プロジェクトを展開し、そのフォルダで：

```sh
python -m pip install "esptool==4.9.0" pyserial
python tools/flash_prebuilt.py --port COM5
python tools/capture.py --port COM5 --output r1_first_test.log
```

COM5は実際のポートへ置き換えてください。Linuxでは `/dev/ttyACM0` 等です。
書き込みとログ表示を同時に同じポートへ接続しないでください。
書き込み速度で接続に失敗する場合は `--baud 115200` を指定できます。
ESPが書き込みモードへ入らない基板では、BOOTを押しながらRESETし、
BOOTを離してから書き込みを開始します。
再起動でポート番号が変わる基板では、ログ収集用に新しいポートを指定してください。

ログ収集を開始してから基板を再起動し、起動からのログを含めてください。
その後の純正アプリでの登録・G2操作は `README_ja.md` の手順を使用します。

## 同梱バイナリの設定

| 項目 | 設定 |
|---|---|
| SoC | ESP32-S3 |
| flash | 4 MB、DIO、40 MHz |
| PSRAM | 不使用 |
| console | ネイティブUSB Serial/JTAG、115200 |
| GPIOボタン | 無効 |
| BLE | NimBLE、3接続枠、MTU優先値247、Coded PHY有効 |
| BLE役割 | peripheral / broadcaster |
| ESP-IDF | 5.5.0 |
| PlatformIO platform | espressif32 6.12.0 |

| アドレス | ファイル |
|---|---|
| 0x00000 | prebuilt/bootloader.bin |
| 0x08000 | prebuilt/partitions.bin |
| 0x10000 | prebuilt/firmware.bin |

オフセットは今回の実際のビルドのflasher_args.jsonと照合しています。
SHA256SUMS.txtは同梱画像のハッシュです。
全flash消去はスクリプトでは行いません。書き込みはbootloader・partition table・
applicationを上書きします。

USB-UARTしか配線されていない基板では、このバイナリのログをその端子で読むことは
できません。ネイティブUSBを接続するか、ソース側のコンソール設定をUARTへ変更して
ビルドしてください。ボタンや基板設定を変更する場合もソースからビルドします。
