# v0.2.1での再試験

## 今回分かったこと

ESP32-S3の起動・広告、相手との接続、2M PHY、MTU 247、channel 2通知の購読、
最初のpairAuth要求の到着まで実機で確認できました。
v0.2が要求を拒否した原因は、受信CRC形式の対応不足です。
要求のCRCは0xB126（モデル全体のMODBUS）で、compact CCITTの0x013Fとは異なります。
このログだけから、認証鍵が不足しているとは判断できません。

## 更新と操作

現在使っているESP-IDF v5.5.1環境で、この版のソースに更新して再ビルド・書き込みしてください。
変更は`src/r1_wire.c`、`include/r1_wire.h`、診断ログの`src/main.c`、
バージョン設定の`CMakeLists.txt`と`include/r1_config.h`です。
ボード固有のsdkconfigや配線設定を変更している場合は、その設定を維持してください。
NVS消去やbond削除は今回のCRC修正には不要です。

1. 起動ログの`PROBE_VERSION=0.2.1`を確認します。
2. 前回と同じ純正iPhoneアプリの登録操作を再試行します。
3. 起動から登録結果・切断までのログを保存します。アプリの表示結果も記録します。

同じ要求が来れば、今回は次のログが期待されます。

```text
MODEL_CHECKSUM conn=... scheme=MODEL_MODBUS
MODEL conn=... module=1 version=100 cmd=00 sub=08 seq=1 status=00 payload=1
PAIR_PHONE security_initiate=...
```

その後の`TX_CH2`、`SECURITY`、`PHONE_AUTH_NOTIFY`、追加の`RX_CH2`、
拒否ログ、切断理由を使い、暗号化・通知・次の登録要求のどこまで進んだか判定します。
これらがすべて出ることや登録が完了することは、まだ確認していません。
G2側の接続が発生した場合は、その接続と`RX_CH1`も含めてください。

実際のflashが8 MBで画像設定が4 MBという警告は、今回のCRC拒否の原因ではありません。
現在のパーティションは先頭4 MB内に収まっているので、この修正のための変更は不要です。
