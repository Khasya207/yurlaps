# 02 — Firmware YurFFB (Arduino)

Firmware ada di [`../firmware/ffb_wheel/`](../firmware/ffb_wheel/):
sketch Arduino murni, **tanpa library eksternal** (hanya Wire/EEPROM bawaan).

```
ffb_wheel/
├── ffb_wheel.ino   # setup/loop, dispatch perintah, kontrol 1 kHz, EEPROM
├── config.h        # ← SATU-SATUNYA file yang perlu kamu ubah
├── protocol.h      # konstanta protokol (identik dgn aplikasi)
├── link.h/.cpp     # framing serial + CRC8
├── motor.h/.cpp    # driver BTS7960 / PWM+DIR, PWM 20 kHz (Timer1)
└── encoder.h/.cpp  # AS5600 / quadrature / potensiometer
```

## 1. Konfigurasi (`config.h`)

Sesuaikan sebelum flash:

| Define | Default | Keterangan |
|---|---|---|
| `ENCODER_TYPE` | `ENCODER_AS5600` | `ENCODER_QUAD` / `ENCODER_POT` utk lainnya |
| `DRIVER_TYPE` | `DRIVER_BTS7960` | `DRIVER_PWM_DIR` utk Cytron/DRV8871 |
| `PWM_20KHZ` | `1` | PWM 20 kHz (senyap) di 328P/32U4 pin 9/10 |
| `SERIAL_BAUD` | `115200` | 250000/500000 utk telemetri lebih rapat |
| `QUAD_CPR` | `2400` | PPR encoder × 4 (hanya utk quadrature) |
| `POT_SPAN_DEG` | `300` | rentang linear pot (hanya utk pot) |
| `DEMO_SELF_TEST` | `0` | 1 = mode pegas center tanpa aplikasi (tes HW) |
| `CTRL_PERIOD_US` | `1000` | periode loop kontrol (1 kHz) |
| `WATCHDOG_MS` | `200` | host diam lebih lama dari ini → torsi 0 |

Pin sudah dipetakan otomatis per board (Uno/Nano, Pro Micro/Leonardo, Mega,
ESP32). Bisa ditimpa di `config.h` bila rangkaianmu beda.

## 2. Flash

1. Buka `ffb_wheel/ffb_wheel.ino` di **Arduino IDE 2.x** (atau VS Code +
   Arduino extension).
2. Pilih board & port yang benar.
3. (Opsi) Untuk tes **tanpa aplikasi & tanpa game**: set `#define DEMO_SELF_TEST 1`
   — setir akan dibuat pegas ke tengah. Kembalikan ke 0 setelah tes.
4. **Upload**.

> Catatan RAM/Flash (Uno): firmware memakai ~13 KB flash dan <1 KB RAM —
> jauh dari batas.

## 3. Tes host (tanpa hardware)

Firmware punya *unit test* berbasis mock Arduino (g++, tanpa board):

```bash
make -C ffb/firmware/test          # menjalankan semua varian
```

Yang diuji: framing+CRC, PING/ENABLE/TORQUE/SET_CONFIG, slew-rate, watchdog,
endstop lunak, invert, fault encoder, EEPROM, decoding quadrature.
Status saat repo ini dibuat: **62/62 check lulus** (AS5600 24, QUAD 19, POT 19).

## 4. Apa yang dilakukan firmware (loop 1 kHz)

1. Baca serial (semua frame valid mereset watchdog).
2. Baca encoder → sudut multi-turn (mdeg) + kecepatan (difilter).
3. Torsi host (dari aplikasi) di-clamp `maxTorque` lalu dihaluskan `slew`.
4. Tambahkan **damper lokal** (redaman fisik, terasa lebih "nyata").
5. **Endstop lunak**: pegas + redaman hanya di luar batas ±`softMax`.
6. Watchdog: tanpa frame dari PC > 200 ms → torsi host = 0 (endstop tetap hidup).
7. Kirim telemetri (default tiap 3 ms ≈ 333 Hz).

Semua parameter (4–6) bisa diubah dari aplikasi dan disimpan ke EEPROM.

## 5. Parameter tuning penting

| Parameter | Efek | Kalau salah |
|---|---|---|
| `maxTorque` (max torsi, mPct) | batas aman keseluruhan | terlalu tinggi → panas/gigi rusak |
| `slew` (mPct/ms) | seberapa cepat torsi boleh berubah | kecil → FFB terasa "lambat/hangat"; besar → benturan kasar |
| `damper` lokal | redaman fisik | terlalu besar → setir berat seperti madu |
| `endstopK` | kerasnya endstop lunak | terlalu kecil → tembus ke stopper fisik |
| `endstopD` | redaman saat menabrak endstop | terlalu kecil → "tok tok tok" |

## 6. Baud rate & telemetri

- 115200 (default) aman untuk semua board. Telemetri 333 Hz sudah cukup untuk
  sumbu game; spring/damper game dihitung di PC dari telemetri ini.
- Kalau mau telemetri lebih rapat: 250000 atau 500000 (0% error clock di AVR
  16 MHz), lalu naikkan baud di aplikasi. Pro Micro (USB CDC) mengabaikan baud.
- Perintah `SET_TELEM` bisa mengubah rate runtime dari aplikasi.

## 7. Kompabilitas board

| Board | Status | Catatan |
|---|---|---|
| Uno / Nano (328P) | ✅ penuh | PWM 20 kHz pin 9/10 (Timer1) |
| Pro Micro / Leonardo (32U4) | ✅ penuh | USB CDC; DTR saat buka port = reset board (normal) |
| Mega 2560 | ✅ | PWM analogWrite biasa (bukan 20 kHz) |
| ESP32 (core 3.x) | ⚠️ terbaik-effort | pin map tersedia di `config.h`; core 2.x tidak punya `analogWrite` |

Untuk board lain: struktur kode portabel (digitalWrite/analogWrite/Wire),
umumnya cukup ganti pin di `config.h`.
