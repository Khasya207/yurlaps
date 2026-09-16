# 01 — Perangkat Keras (Hardware)

Panduan merangkai DIY FFB wheel/joystick dari motor DC ("motor actuator")
dengan komponen yang gampang didapat di Indonesia.

## 1. Konsep dasar

Force feedback = **motor yang didorong bolak-balik oleh game**, dengan sudut
poros yang terbaca terus-menerus. Rantainya:

```
   GAME (PC)
     │  DirectInput FFB ──> vJoy ──> FfbBridge ──> USB serial
     ▼
 ARDUINO (firmware YurFFB, loop kontrol 1 kHz)
     │  PWM + arah                ▲ I2C / interrupt / ADC
     ▼                            │
 DRIVER MOTOR  ◄──────────────────┴── ENCODER (di poros setir)
 (BTS7960 dsb.)                        │
     ▼                                 │
   MOTOR DC  ──────poros/belt────►  SETIR/RIM
```

Game mengirim gaya → motor mendorong rim → tangan kamu merasakan → rim bergerak
→ encoder membaca sudut → game melihat setir berbelok. Lingkaran tertutup.

## 2. Bill of Materials (BOM) — paket paling umum

| Komponen | Rekomendasi | Estimasi harga* | Catatan |
|---|---|---|---|
| Motor DC | **Motor wiper mobil** (12 V) bekas | Rp 100–350 rb | Torsi besar, mur, paling populer utk DIY FFB |
| Alternatif motor | Jig bor 775/550 (24 V) | Rp 80–200 rb | Lebih cepat, torsi lebih kecil — perlu rasio puli |
| Alternatif kelas atas | Motor servo/hoverboard BLDC | Rp 500 rb+ | Butuh driver khusus (di luar cakupan v1) |
| Driver motor | **BTS7960 / IBT-2** (43 A) | Rp 30–60 rb | Dua input PWM (RPWM/LPWM) |
| Alternatif driver | Cytron MD13S / MD10C | Rp 90–150 rb | Mode PWM+DIR, sudah ada di firmware |
| Encoder | **AS5600 modul** (magnetik, I2C) | Rp 20–50 rb | Non-kontak, presisi 12-bit, wajib magnet diametral |
| Alternatif encoder | Potensiometer linear 10k | Rp 10–20 rb | Paling murah, pakai saja kalau sudah ada |
| MCU | **Arduino Nano / Uno** (ATmega328P) | Rp 50–150 rb | Sudah didukung penuh |
| Alternatif MCU | Pro Micro / Leonardo (32U4) | Rp 60–120 rb | USB native — baud berapa pun OK |
| Alternatif MCU | ESP32 (core Arduino 3.x) | Rp 40–100 rb | Pin berbeda — lihat config.h |
| PSU | 12–24 V, **min. 10 A** ( Mean Well LRS-150 dsb.) | Rp 250–450 rb | Ukur arus motor wiper kamu; stall bisa 10 A+ |
| Mekanik | Bearing/flange 17 mm, poros, puli/pinggang, stopper fisik | Rp 100–300 rb | — |

\* Harga pasaran marketplace 2024–2025, hanya estimasi.

> **Aturan praktis PSU:** daya PSU ≥ 2× daya jalan normal motor. Motor wiper
> 12 V bisa menarik 5–15 A saat FFB keras. PSU lemah = FFB "cubit" lalu reset.

## 3. Wiring default (Uno/Nano + BTS7960 + AS5600)

```
   ARDUINO NANO                    BTS7960 (IBT-2)
   ┌──────────┐                    ┌─────────────┐
   │ D9  ─────┼────────────────────┤ RPWM        │
   │ D10 ─────┼────────────────────┤ LPWM        │
   │ D8  ─────┼────────────────────┤ R_EN + L_EN │ (jumper keduanya)
   │          │                    │ OUT+ ───► motor (+)
   │ A4/SDA ──┼─────┐              │ OUT- ───► motor (−)
   │ A5/SCL ──┼──┐  │              │ B+  ───► PSU 12–24 V (+)
   │ GND ─────┼┐│  │              │ B-  ───► PSU (−)
   └──────────┘││  │              └─────────────┘
               ││  └────────────► AS5600 SDA
               │└───────────────► AS5600 SCL
               │
               └──┬────────────► AS5600 GND + BTS7960 GND + PSU GND
                  └── ⚠ GND SEMUA HARUS DISATUKAN (common ground)

   AS5600: VCC→5V, GND→GND, DO pin biarkan (alamat 0x36)
   Magnet: nempel di ujung poros, menghadap chip, celah 0,5–2 mm
```

Poin kritis:

1. **GND Arduino, BTS7960, dan PSU WAJIB disatukan.** Tanpa ini driver bisa
   ngaco atau rusak.
2. Pin EN BTS7960 dijumper (R_EN+L_EN) ke **D8** — firmware bisa mematikan
   motor total (E-STOP / fault).
3. Jangan hidupkan driver sebelum firmware ter-flash dan aplikasi terhubung —
   firmware aman secara default (motor off sampai diberi `ENABLE`).
4. Pin PWM **wajib 9 & 10 di Uno/Nano** karena firmware memakai Timer1 untuk
   PWM 20 kHz (senyap). Kalau pakai driver PWM+DIR (Cytron), pin 10 jadi pin
   arah.

### Varian lain

- **Encoder quadrature** (motor dengan encoder encoder): A→D2, B→D3,
  atur `QUAD_CPR` di `config.h`.
- **Potensiometer**: tap tengah→A0, atur `POT_SPAN_DEG`.
- **Cytron MD13S**: PWM→D9, DIR→D10, set `DRIVER_TYPE 2`.

## 4. Mekanik

- **Motor wiper = direct drive.** Rim pas langsung di poros (pakai adapter
  flange 17 mm → 70 mm PCD universal rim sim-racing, banyak di marketplace).
- **Motor jig bor** butuh reduksi puli/pinggang 1:3–1:5 supaya torsi cukup.
- Pasang **bearing** di kedua sisi poros supaya beban radial tidak ke motor.
- **STOPPER FISIK WAJIB** di batas putaran (mis. ±470° untuk setir 900°).
  Endstop software hanyalah lapisan kedua; jika encoder lepas, motor wiper
  bisa memelintir tangan kamu tanpa stopper fisik.

## 5. Keamanan (baca sebelum menyalakan)

- [ ] Fuse 15–20 A inline di kabel B+ PSU→driver.
- [ ] Stopper fisik terpasang dan kuat.
- [ ] Rim tidak boleh dipegang rapat saat uji pertama — genggam longgar.
- [ ] Mulai dengan **Max torsi 20–30%** di aplikasi, naikkan bertahap.
- [ ] PSU dimatikan saat mengubah wiring.
- [ ] Jangan tinggalkan rig menyala tanpa pengawasan.
- [ ] Siapkan tombol **E-STOP** di aplikasi (Esc) — dan biasakan memakainya.

## 6. Mounting encoder AS5600

1. Magnet **diametral** 6 mm (biasanya ikut modul) ditempel presisi di pusat
   ujung poros (belakang poros, menjauhi rim).
2. Modul dipasang menghadap magnet, celah 0,5–2 mm, chip tepat di pusat.
3. Magnet harus **berputar bersama poros** (jangan miring — jitter).
4. Jauhkan dari bracket besi massal di jalur medan magnet.

Kalau sudut suka "lompat" dekat titik tertentu: magnet kurang center, atau
ada besi di dekatnya. Kalau `ENC-FAULT` muncul di aplikasi: cek kabel
SDA/SCL/GND dan alamat I2C (0x36).

## 7. Alternatif: tidak pakai aplikasi PC (native USB FFB)

Pendekatan lain: board 32U4/STM32 tampil sendiri sebagai **HID FFB joystick**
(tanpa vJoy, tanpa aplikasi). Proyek referensi:

- [denisn73/JoyFFB](https://github.com/denisn73/JoyFFB) — Arduino Pro Micro
  HID FFB joystick.
- [OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) — STM32, matang,
  dukungan driver motor 3-fasa.

YurFFB memilih jalur *serial bridge* karena bisa dipakai dari board murah
(Nano/Uno) dan logika FFB mudah diubah di PC tanpa flash ulang.
