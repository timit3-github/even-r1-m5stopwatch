<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# プロトコルと解析根拠

主な解析基準は公開openCFWのR1 2.2.6.0009 / G2 2.2.6.10です。
現在の操作はEvenアプリ2.3.2 / G2 2.3.2.14 / StopWatchで試験されています。
公開解析、ESP側のエンドポイントログ、実機での操作結果を組み合わせた互換実装です。
純正R1同士の無線通信を取得したものではありません。

## BLEと役割

BAE8系サービスのchannel1をG2操作、channel2をアプリ向けモデル通信に使います。
固定ATT配置はchannel1 RX/TX/CCCD=0x10/0x12/0x13、channel2=0x15/0x17/0x18です。
GAPの追加PPCP特性を無効にしてこの配置を維持し、起動時に実際の値を検査します。
phone/glasses各1役割、物理リンク枠は3です。接続ごとに要求やCCCDイベントから役割を取得します。

phone pairAuthは成功ペイロード00を返信し、暗号化後に非同期認証結果を通知します。
BLE bondはNVSへ保存します。エラー時にNVSを自動消去しません。
advStartの12バイトは2つのpeer候補として保存します。アドレス一致検査は既定で診断用です。
広告は役割不足時に行い、両方の役割が埋まると停止します。
純正のdirected/slow advertisingや遅延切断など全ポリシーを再現していません。

## アプリ向けモデル

断片の5バイトヘッダはsequenceとCRC32です。モデルは12バイトヘッダとpayloadからなります。
受信はモデル全体CRC16/MODBUSと歴史的compact CCITTの両方を検証し、方式をログに残します。
モデル全体CRCの計算時はチェックサム位置10/11をゼロとして扱います。
送信はモデル全体MODBUS方式です。破損・長さ・順序を検証し、受信タイムアウトを設けます。
固定pairAuthテストベクトルはアドレス・鍵・プロフィールを含みません。

module1/command0の状態・バージョン・装着状態・シリアル・pairAuth、設定保存の一部を実装します。
設定GET/SETはstatus bit1で区別します。未知コマンドは実装済みとしてACKしません。
健康設定の保存はセンサー機能を提供するものではありません。

## 旧形式G2コマンド

| Opcode | 要求長 | 応答長 | 主な作用 |
|---|---:|---:|---|
| 0x88 | 4 | 固定応答なし | glasses役割設定、状態通知 |
| 0x85 | 8 | 7 | タッチ有効状態 |
| 0x8A | 6 | 7 | 長押し時間の観測 |
| 0x89 | 8 | 7 | 状態更新 |
| 0x94 | 4 | 5 | 接続維持ACK |

0x88の内部値2は無線上の認証応答ではありません。公開解析で追える内部イベントを基に、
バッテリー0x8Bと装着0x8Cを通知し、架空の固定認証バイトは送信しません。
0x94の応答は00 1A 94 01 01です。SET ACKではbyte4を1にします。
長押し時間の受信値と端末側で使う既定時間は別です。

## 通知

- Battery: 00 09 8B 00 percent charging。chargingは真偽値で、現在は未実装の0です。
- Wear: 00 09 8C 00 wearing。現在の互換実装は装着値1です。
- Touch: 00 09 61 00 type v0 v1 tick:u32LE、11バイト。

type1/2/0は単押し/二度押し/長押し、type4/5は上下、type8は長押し終了です。
新しいtype9はshort→longです。g2flashの受信マッピングとStopWatch実機で確認しています。
受信側の内部SysEvent11とは別なので、BLE上にtype11を送信しません。
スワイプのv0/v1は1/1です。時刻は実際の1024Hz相当カウンターを使います。
古いG2の100tick未満の抑制に合わせ、物理入力には125ms以上の最小間隔を設けます。
人工的に未来の時刻を付けて抑制を回避しません。終了type8は例外扱いです。

EUS deviceStatusの7バイトはpercent/state/01/00/00/00/00です。
stateは1=充電、2=非充電、3=満充電で、旧形式通知の真偽値とは異なります。
現在は残量のみ更新し、充電判定はstate2のままです。

## 参照元

- [openCFW historical reference](https://github.com/kalanihelekunihi/evenRealities-openCFW/tree/832137ec)
- [R1 protocol documentation](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/protocol.md)
- [Legacy command dispatch correlation](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/correlation/LEGACY-COMMAND-DISPATCH-CORRELATION.md)
- [G2 receiver/sender reference](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/g2/components/apollo_main/core_overlay/ring_service.c)
- [Connection control correlation](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/r1/docs/correlation/CONNECTION-CONTROL-CORRELATION.md)
- [Type9 mapping](https://github.com/jimrandomh/g2flash/blob/ca7e0b7a882d50c8ec8e0e597ff93640950a655f/patches/gesture_fwd.c)

参照先のファームウェア、逆コンパイル出力、デバイスの鍵は本配布に含めません。
