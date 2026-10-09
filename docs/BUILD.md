<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# ビルド・接続

## 初回

ESP-IDF 5.5.1の環境でプロジェクトのルートへ移動します。

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor
```

ポートは実機に合わせて変更してください。初回はbootloaderとpartition tableも
書き込むためflashを使います。配布物に古いコンパイル済みバイナリはありません。

## 既存設定からの更新

既存sdkconfigがある場合、sdkconfig.defaultsの値がそのまま反映されないことがあります。
G2が期待する固定GATT配置には、次の4項目がすべて0である必要があります。

```text
CONFIG_BT_NIMBLE_SVC_GAP_PPCP_MIN_CONN_INTERVAL=0
CONFIG_BT_NIMBLE_SVC_GAP_PPCP_MAX_CONN_INTERVAL=0
CONFIG_BT_NIMBLE_SVC_GAP_PPCP_SLAVE_LATENCY=0
CONFIG_BT_NIMBLE_SVC_GAP_PPCP_SUPERVISION_TMO=0
```

既存の他の設定を保持したまま修正できます。

```sh
python tools/configure_gatt_layout.py sdkconfig
idf.py clean
idf.py build
idf.py -p COM5 flash
```

ツールは元の設定をsdkconfig.before-r1-v022へバックアップします。
設定やボードを変更して入力が反映されない場合も、clean後に再ビルドしてください。
通常のソース更新でNVS消去やペアリング解除は不要です。

## 接続確認

起動時、GATT_LAYOUTに次が出ることを確認します。不一致時は広告を停止します。

| Channel | RX | TX | CCCD |
|---|---|---|---|
| 1 | 0x10 | 0x12 | 0x13 |
| 2 | 0x15 | 0x17 | 0x18 |

EvenアプリでR1として登録します。G2の再接続には数分かかる場合があります。
statusコマンドでrole=glasses、ch1=1の接続があることを確認してください。
role=phone、ch1=0、ch2=1だけなら、アプリとの接続だけでG2の操作リンクはありません。
広告開始時にstatusやautoを入力する必要はありません。

別の基板へ移行するとBLEアドレスが変わるため、アプリで新しいリモコンとして登録します。
手動selectを使った場合、autoで送信先の手動選択を解除できます。

## ログ収集

idf.py monitorはターミナル操作や入力方式に注意し、コマンドの末尾にEnterを入力します。
端末からのコマンド送信とタイムスタンプ付き収集を行う補助ツールもあります。

```sh
python -m pip install pyserial
python tools/capture.py --port COM5 --output captures/session.log
```

先にcapturesフォルダを作成してください。既存ログは上書きしません。
monitorとcapture.pyを同時に同じポートへ接続しないでください。
ログにはBLEアドレスや設定内容が含まれるため、公開時は該当部分を伏せてください。

## テスト

```sh
sh tests/run_host_tests.sh
```

C11対応コンパイラccとPOSIX shellが必要です。WindowsではWSLなどで実行できます。
ESP-IDFでのビルドや実機試験とは別のテストです。
