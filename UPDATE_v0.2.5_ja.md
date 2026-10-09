# v0.2.5: short tap → long tapの専用通知

v0.2.4の二度押しはG2実機で認識しました。一方、短押し→長押しは認識しませんでした。
提供ログではtype 1、type 0、type 8の順にESPが通知しています。
type 1の時刻0x477ffからtype 0の0x47891までは146ticks（約142.58ms）。
旧G2の100ticks未満の抑制条件には入らず、解放まで約1.07秒ありました。
ログはESP側の送信を示すもので、G2側の処理完了を示すものではありません。

## 新しく確認したイベント

新しいg2flashのR1受信解析では、BLE通知type 9がtap-then-longに対応しています。
READMEもG2 2.2.9以降の独立ジェスチャーとして記載しています。
以前の実装は2.2.6の受信表を基準にし、この新しい通知が抜けていました。

解析コード:
https://github.com/jimrandomh/g2flash/blob/ca7e0b7a882d50c8ec8e0e597ff93640950a655f/patches/gesture_fwd.c

`faceclaw_ring_report` はreport[4]==9をCFW内部のSysEvent 11へ対応付けています。
R1のBLE通知番号9とCFWのイベント番号11は別物です。ESPから11を送りません。
このコードの受信側の対応表を参照した修正で、G2にCFWを書き込む操作は行いません。

v0.2.5では、短押し後300ms以内に再び押し200ms保持すると、type 9を1回送ります。
単押しtype 1と通常の長押しtype 0は、この連続操作では送りません。
解放はtype 8のままです。単押し・二度押し・単独長押し・回転は従来の判定を維持します。
300msと200msはM5Dialの操作を分類する調整値です。

## 更新

v0.2.4から次の5ファイルを更新します。

- CMakeLists.txt
- src/main.c
- src/r1_inputs.c
- src/r1_legacy.c
- include/r1_inputs.h

include/r1_config.hのR1_PROBE_VERSIONを"0.2.5"へ変更します。
自分で変更したR1_APP_VERSIONやR1_ENCODER_REVERSE、sdkconfig、NVSは維持してください。
新しい設定項目の追加はありません。差分はPATCH_v0.2.4_to_v0.2.5.diffに同梱しました。

今回の配布のR1_APP_VERSION既定値は"2.3.2.9999"です。
ユーザー実機のEvenアプリ2.3.2で「互換性のないリングバージョン」の警告が消えました。
既に設定済みならその値を維持してください。警告の内部比較条件は未解析です。
この表示値は互換実験用で、実在する純正R1のファームウェア版を示すものではありません。

```sh
idf.py clean
idf.py build
idf.py -p COM5 app-flash
idf.py -p COM5 monitor
```

COM5は実際のポートへ置き換えます。起動時PROBE_VERSION=0.2.5を確認してください。

## 確認

最初にコンソールでm＋Enter（menu＋Enterでも同じ）を送り、G2のメニューが開くか
確認します。1秒程度後にr＋Enterで解放イベントを送ります。
`event 9 0 0`も同じtype 9です。

```text
TOUCH type=9 v0=0 v1=0 queued=1
TX_CH1 conn=... len=11
00 09 61 00 09 00 00 xx xx xx xx
```

続いて短押しして離し、300ms以内に再び押して1秒程度保持します。
期待ログはBUTTON events=16 → TOUCH type=9 → TX_CH1 len=11。
解放ではBUTTON events=4 → TOUCH type=8です。
この操作中にtype 1やtype 0が出ていたら、更新ファイルと起動バージョンを確認してください。

コンソールmと物理操作のそれぞれについて、メニューが開いたか確認してください。
mでも動かない場合は、その送信前後のRX_CH1 / TOUCH / TX_CH1ログとG2の画面状況を
比較します。新しい解析の対応表だけでG2 2.3.2.14の全受付条件が確定したわけではありません。

## 検証範囲

ホストのwire/legacy/inputテストを実施。type 9の11バイト形式、未対応type 11の拒否、
連続操作が専用イベントだけになること、二度押し・通常長押し・解放・境界時刻・
遅延ポーリング・時刻の桁あふれを確認しました。
ESP向けビルドとM5Dial/G2でのtype 9動作は未確認です。ソースのみの配布です。
