# 05 — Kompatibilitas & Setup Game

## 1. Prinsip

YurFFB + vJoy tampil sebagai **joystick DirectInput dengan FFB**. Artinya,
semua game yang mendukung setir/joystick DirectInput FFB **bisa langsung
memakainya** — di menu kontrol game akan muncul sebagai **"vJoy Device"**.

Permainan konsol-style yang **hanya** XInput (gamepad Xbox) tidak akan
melihatnya.

## 2. Game yang teruji komunitas (DirectInput FFB)

Daftar berikut mengikuti pengalaman komunitas DIY FFB (klasifikasi mengacu
wiki OpenFFBoard untuk game DirectInput FFB — [Games setup ·
Ultrawipf/OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard/wiki/Games-setup)):

| Game | Status umum | Catatan |
|---|---|---|
| Assetto Corsa | ✅ | paling gampang; set rotation 900° |
| Assetto Corsa Competizione | ✅ | — |
| iRacing | ✅ | — |
| rFactor 2 | ✅ | — |
| BeamNG.drive | ✅ | — |
| Euro Truck Simulator 2 / ATS | ✅ | — |
| Live for Speed | ✅ | klasik, FFB DirectInput murni |
| RaceRoom | ✅ | — |
| Project CARS 1/2 | ✅ | — |
| F1 2018–2024 | ✅ | — |
| Wreckfest | ✅ | — |
| Richard Burns Rally | ✅ | +plugin FFB lebih bagus |
| DiRT Rally 1/2, DiRT 4 | ☑️ | butuh ubah config file (sama seperti wheel DIY lain) |
| EA WRC | ☑️ | idem |
| Forza Horizon 4/5, Forza Motorsport | ☑️ | perlu trik whitelist seperti wheel DIY lain |
| WRC 8/Generations | ✅ | FFB-nya pakai **spring** — naikkan Spring gain |
| MotoGP, game XInput-only | ❌ | tidak mendukung DirectInput FFB |

> Intinya: kalau game-nya menerima wheel Logitech/Thrustmaster DirectInput,
> hampir pasti menerima vJoy Device.

## 3. Setup umum di game

1. **Controls** → pilih `vJoy Device`.
2. Setir: putar rim kiri-kanan → game membaca sumbu X. Kalau terbalik:
   **Invert encoder** di FfbBridge (jangan invert di game bila bisa hindari,
   biar FFB dan sumbu konsisten).
3. Pedal/gas-rem: vJoy device lain bisa dibuat untuk pedal
   (Configure vJoy device #2, sumbu Y/RX) — di luar cakupan v1 YurFFB
   (v1 = setir + FFB).
4. **FFB game**:
   - Gain/overall force: mulai 60–80%, naikkan sampai `CLAMP` muncul di
     telemetri FfbBridge saat manuver keras, lalu turunkan 10%.
   - **Spring effect game: 0%** (kecuali WRC dsb. yang FFB-nya spring-based).
   - **Damper effect game: 0–20%** — setir DIY punya gesekan asli.
   - "Constant 100%" wajar; jangan takut clipping ditangani min force.
5. Set **soft lock / rotation** game = **Rotasi kemudi** FfbBridge.

## 4. Diagnosa "FFB tidak keluar"

1. `joy.cpl` → vJoy Device muncul? Tidak → vJoy belum ter-install/belum
   di-enable.
2. FfbBridge: tekan **Torsi ➡** — rim bergerak? Tidak → masalah di sisi
   firmware/motor (cek `ENC-FAULT`, kabel, PSU), bukan game.
3. Telemetri: `Efek aktif` berubah saat masuk game (mis. `EtConst PLAY`)?
   Tidak → game tidak mengirim FFB ke vJoy — cek game memilih device yang
   benar dan FFB enabled di game.
4. `WATCHDOG` nyala terus → masalah serial (kabel, baud, CPU PC sibuk berat).

## 5. Batasan vJoy yang perlu diketahui

- vJoy melaporkan **effect block index yang sama** untuk semua efek pada
  beberapa versi (umumnya semua = 1). Akibatnya, bila sebuah game memainkan
  beberapa efek sekaligus (constant + rumble), efek terakhir yang di-update
  bisa menimpa. FfbBridge menangani kasus umum (constant force utama) dengan
  benar; game yang mengandalkan banyak efek simultan mungkin terasa kurang
  kaya dibanding wheel komersial.
- Windows saja (vJoy tidak ada di Linux/macOS). Alternatif Linux: hidpp /
  uinput + evdev (roadmap, belum dibuat).
- FFB "true rate" 500–1000 Hz milik wheel high-end; di sini update torsi
  500 Hz + latensi serial ~beberapa ms — sudah bagus, tapi bukan Simucube.

## 6. Mengapa bukan mode "gamepad vibration"?

Rumble gamepad (XInput) = getaran on/off sederhana tanpa arah/torsi —
tidak cocok untuk setir. DirectInput FFB (yang kita pakai) memungkinkan gaya
berarah berkekuatan kontinu: inilah yang membuat mobil terasa "hidup".
