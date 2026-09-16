# FfbBridge — aplikasi PC untuk YurFFB

Aplikasi Windows (C#/.NET 8, WinForms) yang menjembatani **Force Feedback DirectInput
dari game** ke **firmware YurFFB** di Arduino:

```
Game ──(DirectInput FFB)──> vJoy (setir virtual) ──> FfbBridge
                                                        │  hitung gaya 500 Hz
                                                        ▼
                                            Serial ──> Firmware ──> BTS7960 ──> Motor
                                            Serial <── (sudut, status) <── AS5600
                                                        │
                              FfbBridge ──> sumbu X vJoy <──┘ (game melihat setir)
```

Tanpa aplikasi ini, game tidak bisa "berbicara" dengan motor DC — vJoy-lah yang
membuat Windows melihat **setir FFB biasa**, dan FfbBridge yang menerjemahkan
efek FFB (constant force, spring, damper, rumble) menjadi torsi motor.

## Prasyarat

1. **Windows 10/11** (64-bit).
2. **[vJoy](https://github.com/shauleiz/vJoy) versi 2.1.8 atau lebih baru**
   (driver joystick virtual dengan dukungan FFB). Install → restart → jalankan
   **Configure vJoy**:
   - Device 1: **Enabled**
   - Axes: centang minimal **X**
   - Simpan (perubahan butuh replug vJoy / restart kalau tidak jalan)
3. **.NET 8 SDK** kalau mau build sendiri, atau pakai hasil `dotnet publish`.

## Build

```bash
# dari folder ini (ffb/app)
dotnet build -c Release

# atau buat EXE mandiri (tidak butuh .NET runtime di PC lain):
dotnet publish FfbBridge -c Release -r win-x64 --self-contained -o publish
```

Bisa juga dibuka langsung dengan **Visual Studio 2022** (`FfbBridge.sln`).

> Build di Linux/macOS hanya untuk cek kompilasi:
> `dotnet build -p:EnableWindowsTargeting=true` (aplikasi tetap hanya jalan di Windows).

## Cara pakai

1. Flash firmware YurFFB ke Arduino (lihat `../docs/02-firmware.md`), rangkai hardware
   (`../docs/01-hardware.md`), pasang USB.
2. Jalankan `FfbBridge.exe`.
3. **Deteksi Otomatis** → pilih port → **Hubungkan**. Status harusnya:
   `YurFFB fw 1.0 · AS5600 · BTS7960`.
4. **Aktifkan vJoy** (device #1).
5. Centang **FFB aktif**.
6. Uji dulu dengan tombol **Torsi / Sine / Spring** (pegang rim!).
7. Di game (mis. Assetto Corsa): pilih *vJoy Device* sebagai setir → set rotation
   sesuai pengaturan **Rotasi kemudi** di FfbBridge.

## Kontrol penting

| Kontrol | Fungsi |
|---|---|
| Gain global | skala seluruh gaya (mulai rendah, mis. 30–50% saat tuning) |
| Min force | kompensasi motor yang "mati" di gaya kecil (gesekan) |
| Max torsi | batas aman torsi yang dikirim ke motor |
| Rotasi kemudi | rentang derajat yang dipetakan ke sumbu X game (900° standar) |
| Auto-center idle | pegas center saat game tidak mengirim efek |
| Endstop lunak / K / D | endstop software di firmware (tetap pasang stopper fisik!) |
| Invert motor / encoder | perbaiki arah jika torsi/sumbu terbalik |
| **E-STOP (Esc)** | matikan motor seketika |

## Telemetri

| Flag | Arti |
|---|---|
| `WATCHDOG` | firmware tidak menerima data dari PC (>200 ms) — torsi host dipaksa 0 |
| `DISABLED` | motor belum di-enable |
| `ENDSTOP` | sedang menekan endstop lunak |
| `ENC-FAULT` | encoder tidak terbaca — **motor ditolak enable** |
| `CLAMP` | torsi dibatasi (maxTorque/endstop saturation) |

## Troubleshooting

- **"vJoyInterface.dll tidak ditemukan"** → vJoy belum terinstall; install dan
  restart. Aplikasi memuat DLL 64-bit dari System32.
- **"vJoy #N dipakai proses lain"** → tutup aplikasi lain yang menguasai vJoy
  (Joystick Gremlin, feeder lama, dsb.).
- **"tidak punya sumbu X"** → jalankan *Configure vJoy*, centang X, OK.
- Game tidak menampilkan vJoy → cek di `joy.cpl` (Set up USB game controllers)
  apakah "vJoy Device" muncul.
- FFB terasa terbalik → centang **Invert motor**.
- Sumbu terbalik → centang **Invert encoder**.
- Setir "mati" di gaya kecil → naikkan **Min force** sedikit demi sedikit.

## Struktur kode

| File | Isi |
|---|---|
| `FfbBridge/Protocol.cs` | framing + CRC8 + struct config/state (mirror `protocol.h`) |
| `FfbBridge/SerialLink.cs` | port serial, parser frame, auto-detect YurFFB |
| `FfbBridge/VJoy.cs` | P/Invoke `vJoyInterface.dll` + callback FFB (helper `Ffb_h_*`) |
| `FfbBridge/FfbEngine.cs` | mesin efek 500 Hz: constant/ramp/periodic/condition + gain |
| `FfbBridge/MainForm.cs` | UI |
| `FfbBridge/Settings.cs` | settings JSON (`%APPDATA%\FfbBridge\settings.json`) |
