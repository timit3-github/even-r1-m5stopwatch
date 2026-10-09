Protocol algorithms and layouts were implemented using the MIT-licensed openCFW
project's documentation and historical reference code:
https://github.com/kalanihelekunihi/evenRealities-openCFW
Historical reference commit: 832137ec (2026-09-29 cleanup predecessor).
The copyright holders of that project retain their rights in the reference work.

The v0.1 smartphone gesture interpretation was checked against MentraOS's R1.kt (Apache-2.0 project):
https://github.com/Mentra-Community/MentraOS/blob/dev/mobile/modules/bluetooth-sdk/android/src/main/java/com/mentra/bluetoothsdk/controllers/R1.kt
No MentraOS implementation code or firmware binaries are included.

In v0.2 the 3-byte smartphone format is replaced by the independently implemented
legacy G2 format, based on the old stock R1 producer and G2 receiver evidence.
See PROTOCOL_NOTES.md for addresses, primary source links, and limitations.

This experiment is unofficial and unaffiliated with Even Realities.
