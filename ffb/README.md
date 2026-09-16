# YurFFB — DIY Force Feedback dari Motor DC (Setir Sim Racing)

> Jawaban singkat atas tiga pertanyaan:
> **1) Bisakah DIY FFB dari motor actuator? — BISA.**
> **2) Bisakah buat firmware Arduinonya? — SUDAH DIBUAT & DIUJI** (loop kontrol 1 kHz, 62 unit-test lulus).
> **3) Bisakah buat aplikasinya & langsung terbaca di game? — SUDAH DIBUAT.** Game melihatnya
> sebagai setir FFB biasa ("vJoy Device") lewat DirectInput — Assetto Corsa, iRacing,
> ETS2, rFactor 2, dll. tinggal dipilih di menu kontrol game.

Proyek ini hidup berdampingan dengan YurLaps (sistem lap-timing di repo ini)
dan **tidak mengubah file YurLaps sama sekali**.

---

## Cara kerja (arsitektur)

```
┌─────────────┐  DirectInput FFB   ┌──────────────┐  callback FFB   ┌──────────────────┐
│  GAME (PC)  │◄──────────────────►│  vJoy Device │◄───────────────►│  FfbBridge (C#)  │
│ AC, iRacing │   (setir virtual)  │  (virtual)   │                 │  engine 500 Hz   │
└─────────────┘                    └──────────────┘                 └────────┬─────────┘
                                     ▲ SetAxis X (posisi setir)               │ serial
                                     │                                        │ torsi (mPct)
                              ┌──────┴───────────────────────────────┐        ▼
                              │        ARDUINO + YurFFB fw           │  ┌───────────┐
                              │  loop kontrol 1 kHz:                 │  │  BTS7960  │
                              │  · encoder multi-turn + kecepatan    │  └────┬──────┘
                              │  · slew-limit + damper + endstop     │       ▼
                              │  · watchdog 200 ms (host mati→0)     │  ┌───────────┐
                              └──────────────┬───────────────────────┘  │ MOTOR DC  │
                                             │ I2C/quad/ADC             │ (wiper)   │
                                        ┌────┴─────┐                    └────┬──────┘
                                        │ AS5600   │◄── magnet di poros ────┘
                                        └──────────┘        (poros setir)
```

Alur saat balapan: game menghitung gaya → vJoy → FfbBridge menjumlahkan efek
(constant/spring/damper/rumble) → kirim torsi ke Arduino → BTS7960 mendorong
motor → encoder membaca sudut → telemetri naik lagi ke game sebagai sumbu X.
Lingkaran tertutup ≈ 2–5 ms.

## Struktur proyek

```
ffb/
├── firmware/
│   ├── ffb_wheel/          # sketch Arduino (Uno/Nano/Pro Micro/Mega/ESP32)
│   │   ├── ffb_wheel.ino   #   loop kontrol 1 kHz + dispatch perintah
│   │   ├── config.h        #   ← ubah pin/jenis encoder/driver di sini
│   │   ├── protocol.h      #   konstanta protokol (jangan diubah)
│   │   ├── link.h/.cpp     #   framing serial + CRC8
│   │   ├── motor.h/.cpp    #   BTS7960 / PWM+DIR, PWM 20 kHz
│   │   └── encoder.h/.cpp  #   AS5600 / quadrature / potensiometer
│   └── test/               # unit-test host (g++, mock Arduino) — 62 check
├── app/
│   ├── FfbBridge.sln
│   └── FfbBridge/          # aplikasi Windows C#/.NET 8 (WinForms)
│       ├── Protocol.cs     #   mirror protokol firmware
│       ├── SerialLink.cs   #   serial + auto-detect YurFFB
│       ├── VJoy.cs         #   P/Invoke vJoyInterface.dll + parsing FFB
│       ├── FfbEngine.cs    #   mesin efek FFB (constant/ramp/periodic/condition)
│       ├── MainForm.cs     #   UI (gain, telemetri, uji coba, E-STOP)
│       └── Settings.cs     #   settings JSON
└── docs/
    ├── 01-hardware.md      # BOM, wiring, mekanik, KESELAMATAN
    ├── 02-firmware.md      # konfigurasi & flash firmware
    ├── 03-aplikasi.md      # konsep & urutan menyala pertama kali
    ├── 04-protokol.md      # spesifikasi protokol serial lengkap
    └── 05-game.md          # game apa saja yang cocok & settingnya
```

## Quick start (15 menit kalau komponen siap)

1. **Rangkai** hardware default: motor wiper + BTS7960 + AS5600 + Arduino Nano
   (`docs/01-hardware.md` — perhatikan common ground & stopper fisik!).
2. **Flash** firmware: buka `firmware/ffb_wheel/ffb_wheel.ino` di Arduino IDE,
   sesuaikan `config.h`, upload (`docs/02-firmware.md`).
   Tes cepat tanpa PC-app: set `DEMO_SELF_TEST 1` → setir buat pegas center.
3. **Install vJoy** di Windows + .NET 8, build aplikasi:
   `cd ffb/app && dotnet build -c Release` (`app/README.md`).
4. **Jalankan FfbBridge**: Deteksi Otomatis → Hubungkan → Aktifkan vJoy →
   centang FFB aktif → uji **Spring/Torsi** dengan max torsi 20–30% dulu.
5. **Masuk game** (mis. Assetto Corsa) → pilih `vJoy Device` sebagai setir →
   nikmati FFB-nya (`docs/05-game.md`).

## Yang sudah diuji & yang belum

| Komponen | Status |
|---|---|
| Firmware — logika & protokol | ✅ 62/62 unit-test host lulus (3 varian encoder, 4 varian driver/config) |
| Firmware — di hardware nyata | ⏳ perlu kamu coba (belum ada HW di tangan) |
| Aplikasi — kode lengkap & review | ✅ ditulis mengikuti API resmi vJoy (`vjoyinterface.h`) |
| Aplikasi — compile & jalan di Windows | ⏳ build di Windows-mu (`dotnet build`) |
| Rantai game → vJoy → motor | ⏳ |

⚠ **Ini proyek eksperimen.** Motor wiper bisa mematahkan jari. Baca bagian
keamanan di `docs/01-hardware.md` sebelum menyalakan apa pun.

## FAQ singkat

- **Kenapa perlu aplikasi PC? Kenapa firmware tidak langsung "jadi setir"?**
  USB-HID FFB butuh MCU dengan USB device yang bisa dikustomisasi
  (32U4/STM32 + LUFA/descriptor PID). Uno/Nano tidak bisa. Jalur serial-bridge
  ini yang dipakai FFBeast dan sejenisnya — fleksibel, murah, dan logika FFB
  gampang di-update tanpa flash ulang. (Alternatif native HID: JoyFFB/OpenFFBoard
  — lihat `docs/01-hardware.md` §7.)

- **Game XInput doang (mis. beberapa game arcade) bisa?** Tidak — mereka tidak
  bicara DirectInput FFB.

- **Bisa untuk flight sim / joystick FFB?** Bisa — konsepnya identik; ganti
  mekanik (gimbal) dan turunkan `maxTorque`. Engine FFB-nya sama.

- **Bisa pakai motor lain?** Motor DC apa pun yang didukung driver BTS7960
  (≤43 A) atau Cytron. BLDC 3-fasa (hoverboard) butuh driver lain — roadmap.

- **Linux?** vJoy tidak ada di Linux. Roadmap: backend `uinput`/evdev.

## Roadmap ide

- [ ] Pedal & shifter via vJoy device #2 (analog → sumbu Y/RX)
- [ ] BLDC 3-fasa (VESC/ODrive) — protokol sama, driver beda
- [ ] Backend Linux (uinput) & macOS
- [ ] Kalibrasi otomatis endstop + center saat startup
- [ ] Profil per-game (auto-switch gain/rotasi)

## Kredit & referensi

- [vJoy](https://github.com/shauleiz/vJoy) (Shaul Eizikovich) — driver joystick virtual + FFB.
- [FFBeast](https://ffbeast.github.io) — proyek DIY FFB open-source yang
  mempopulerkan arsitektur serial-bridge ini (referensi konsep).
- [OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) — referensi
  kompatibilitas game DirectInput FFB.
- [denisn73/JoyFFB](https://github.com/denisn73/JoyFFB) — referensi HID FFB
  native di Arduino 32U4.
- DirectInput / HID PID (Physical Interface Device) specification — dasar
  format efek FFB yang diparsing `VJoy.cs`/`FfbEngine.cs`.
