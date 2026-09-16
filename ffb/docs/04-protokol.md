# 04 — Spesifikasi Protokol Serial (YurFFB v1)

Dua implementasi yang **harus tetap identik**:

- Firmware: [`../firmware/ffb_wheel/protocol.h`](../firmware/ffb_wheel/protocol.h)
- Aplikasi: [`../app/FfbBridge/Protocol.cs`](../app/FfbBridge/Protocol.cs)

## 1. Framing

```
┌──────┬──────┬─────┬───────┬─────────────────┬──────┐
│ 0xAA │ 0x55 │ LEN │  CMD  │  PAYLOAD (N b)  │ CRC8 │
└──────┴──────┴─────┴───────┴─────────────────┴──────┘
  SOF1   SOF2   u8     u8        0..38 B        u8

LEN  = 1 + N + 1  (CMD + payload + CRC; rentang 2..40)
CRC8 = poly 0x07, init 0x00, tanpa refleksi/xorout  (= CRC-8/SMBUS;
        test vector: crc8("123456789") == 0xF4)
      dihitung atas byte CMD + PAYLOAD.
Semua field multi-byte little-endian.
```

Parser RX harus bisa *resync*: buang byte sampai menemukan `AA 55` baru,
frame dengan CRC salah dibuang diam-diam.

## 2. Perintah host → device

| CMD | Nama | Payload | Keterangan |
|---|---|---|---|
| 0x01 | PING | — | balas PONG |
| 0x02 | ENABLE | u8 on | 1 = motor aktif. Ditolak bila encoder fault |
| 0x03 | TORQUE | i16 mPct | −1000..+1000; dikirim ~500 Hz, sekaligus keep-alive watchdog |
| 0x04 | SET_CONFIG | 16 B | lihat §4; firmware membalas CONFIG (echo) |
| 0x05 | GET_STATE | — | balas satu STATE langsung |
| 0x06 | SET_CENTER | — | posisi sekarang = 0° |
| 0x07 | SET_TELEM | u16 ms | 0 = telemetri off (default firmware 3 ms) |
| 0x08 | SAVE_CONFIG | — | simpan config ke EEPROM |
| 0x09 | RESET_CONFIG | — | muat default + hapus EEPROM, balas CONFIG |

## 3. Pesan device → host

### 3.1 PONG (0x81) — 11 byte

```
[0..5]  ASCII "YURFFB"
[6]     protoVer (1)
[7]     fwMajor
[8]     fwMinor
[9]     encoderType: 1=AS5600, 2=quadrature, 3=pot
[10]    driverType:  1=BTS7960(2×PWM), 2=PWM+DIR
```

### 3.2 STATE (0x82) — 15 byte

```
[0..3]  u32  seq          (nomor urut telemetri)
[4..7]  i32  angle_mdeg   (multi-turn, relatif center)
[8..9]  i16  vel_ddeg_s   (deci-derajat/detik)
[10..11] i16 torque_mPct  (perintah terakhir ke motor, termasuk efek lokal)
[12]    u8   flags
[13..14] u16 loop_us      (durasi loop kontrol terakhir)
```

flags: `0x01 WATCHDOG · 0x02 DISABLED · 0x04 ENDSTOP · 0x08 ENC-FAULT · 0x10 CLAMPED`

### 3.3 LOG (0x83)

ASCII ter-NUL-terminated maks. 47 byte.

### 3.4 CONFIG (0x84) — 16 byte, layout sama SET_CONFIG

## 4. Layout CONFIG (16 byte)

| Off | Tipe | Field | Rentang wajar |
|---|---|---|---|
| 0 | u16 | maxTorque (mPct) | 100..1000 |
| 2 | i16 | slew (mPct/ms) | 5..100 |
| 4 | i16 | softMin (°) | −(sudut stopper fisik)−margin |
| 6 | i16 | softMax (°) | +… |
| 8 | i16 | endstopK (mPct/°) | 30..150 |
| 10 | i16 | endstopD (mPct per °/s) | 5..50 |
| 12 | i16 | damper lokal (mPct per °/s) | 0..10 |
| 14 | u8 | flags | bit0 invert motor, bit1 invert encoder |
| 15 | u8 | reserved | 0 |

## 5. Contoh sesi (heksadesimal)

```
host: AA 55 03 01 39            PING
dev : AA 55 0D 81 59 55 52 46 46 42 01 01 00 01 01 xx   PONG "YURFFB" fw 1.0
host: AA 55 04 02 01 28         ENABLE(1)
host: AA 55 05 03 E8 01 56      TORQUE(+488)   (i16 LE)
dev : AA 55 11 82 ...           STATE (15B payload)
```

## 6. Perilaku keselamatan firmware

- **Watchdog 200 ms**: tanpa frame valid dari host, torsi host dipaksa 0.
  Endstop lunak & damper lokal tetap aktif (roda tidak "lepas" di batas).
- **ENABLE ditolak** selama encoder fault → motor tidak bisa menyala buta.
- Torsi selalu di-clamp `±maxTorque` dan `±1000` (100% duty).
- `maxTorque`/`slew` membatasi laju perubahan → tidak ada lonjakan PWM.
- TX telemetri **tidak pernah memblok loop kontrol** (frame dibuang bila
  buffer TX penuh).
