# v0.2.2: G2接続後のATTハンドル配置

## v0.2.1のログから確認できたこと

iPhone接続はphone役割、暗号化1、bond保存1、pairAuth通知送信まで進みました。
ユーザーからアプリ上のペアリング完了も報告されています。
advStartの2つ目の接続先と一致するpublicアドレスの相手が接続したため、
この相手はG2と考えられます。MTU23のまま、購読・RX_CH1なしで切断しました。
PHYはtx=1M / rx=Codedです。role=unknownは役割イベント未到着の結果です。
役割だけを強制しても、購読なしではG2に操作通知を届けられません。

## 最有力候補: 固定ATTハンドルとの不一致

公開旧G2 ble_ring_profile.cは接続時にRX書き込み値0x10、TX通知値0x12、
CCCD 0x13をセットし、CCCDへ500/700/900ms後にWrite Requestします。
これらはUUIDの末尾ではなくATTデータベース上のハンドル番号です。

ESP-IDF v5.5.1が参照するesp-nimbleコミット
b45dcedcafb7888174c3567002c36b342ec0b723の標準サービスを確認しました。
今回の設定ではGAPが7属性、GATTが8属性、次にBAE8が登録されます。
ユーザーのTX_CH2 att_handle=25とも整合します。

| 項目 | v0.2.1配置（SDKから計算） | v0.2.2の期待配置 |
|---|---:|---:|
| channel 1 RX値 | 0x12 | 0x10 |
| channel 1 TX値 | 0x14 | 0x12 |
| channel 1 CCCD | 0x15 | 0x13 |
| channel 2 RX値 | 0x17 | 0x15 |
| channel 2 TX値 | 0x19 | 0x17 |
| channel 2 CCCD | 0x1A | 0x18 |

v0.2.1の0x13は通知CCCDではなく特性宣言です。旧G2と同じ固定ハンドルを
現行G2も使用するなら、そこへの購読書き込みが失敗します。
現行G2のATT Write Requestそのものは今回のログにはないため、原因の確定は
修正後の購読ログで行います。GAP PPCPの4値を全て0にすると追加特性の2属性が
なくなり、期待配置になります。接続interval/latency/timeoutを全て0にする操作
ではなく、希望接続パラメーターを公開するGAP特性を外す設定です。

## 更新

FLASH_ja.mdの手順で既存sdkconfigも更新してビルドします。
4値が残っている場合はビルドエラーで更新方法を表示します。
SDKや他の標準特性が違い、配置が合わない場合は起動時に
GATT_LAYOUT_MISMATCHを出して広告を止めます。その場合は起動ログを送ってください。

期待する起動ログ:

```text
GATT_LAYOUT rx1=0010 tx1=0012 cccd1=0013 rx2=0015 tx2=0017 cccd2=0018
PROBE_VERSION=0.2.2
```

1. 書き込み後、G2とiPhoneのBluetooth接続を再接続します。
2. iPhone側が従来どおり接続し、G2接続の後にSUBSCRIBE ch1=1と
   CCCD_GLASSES_ROLE_EVENTが出るか確認します。
3. RX_CH1の要求・応答まで進んだらstatusでrole=glasses / ch1=1を確認します。
4. コンソールのsとEnterで単押しを送信します。TX_CH1と実際のG2表示変化を記録します。
   MTU23でも11バイトの操作通知は収まります。

iPhoneの通知ハンドルも変わるため、古いGATTキャッシュが問題になる可能性があります。
暗号化されたbond済み接続で標準Service Changedを通知予約しますが、
相手が購読していなければ届きません。iPhone側まで接続・応答が止まった場合は、
純正Evenアプリから試作R1を解除して再追加してください。iOSのBluetooth設定に
試作R1が残っている場合は、その試作R1だけを登録解除して再ペアリングします。
G2自体の登録解除やNVS全消去は最初の手順に含めません。

未対応のmodule 2 / cmd06 / sub02とgetAlgoKeyStatus(sub0B)も観測しました。
意味や必要性を確定できていないため、成功応答を捏造せず引き続きログに残します。
今回のchannel 1未購読との因果関係は、ログだけからは確定できません。

次に必要なログは、起動のGATT登録一覧・GATT_LAYOUT、iPhone再接続、G2接続、
SUBSCRIBE、RX_CH1、TX_CH1、status、切断までです。アプリの表示とG2の反応も記録してください。
