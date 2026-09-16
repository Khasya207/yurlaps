# 03 — Aplikasi FfbBridge (Windows)

Lihat [`../app/README.md`](../app/README.md) untuk build & pemakaian harian.
Dokumen ini menjelaskan konsep dan urutan benar saat pertama kali menyalakan.

## 1. Kenapa perlu vJoy?

Game PC (DirectInput) hanya bisa mengirim FFB ke **perangkat HID yang
menyatakan dirinya FFB-capable**. Arduino Uno biasa tampil sebagai port serial
— game tidak mengenalnya. vJoy membuat **setir virtual** di Windows yang:

- menerima **input sumbu** dari aplikasi kita (posisi setir nyata), dan
- mengirim **efek FFB dari game** ke aplikasi kita lewat callback.

Jadi game melihat `vJoy Device` seperti Logitech biasa. Solusi yang sama
dipakai FFBeast dan komunitas DIY FFB lainnya.

## 2. Urutan menyala pertama kali (PENTING)

1. Rangkaian rapi, stopper fisik terpasang, fuse terpasang.
2. **Tangan JAUH dari rim** saat pertama kali enable — pegang hanya bila
   torsi sudah terasa kecil dan arahnya benar.
3. Nyalakan PSU → USB Arduino → jalankan FfbBridge.
4. Deteksi → Hubungkan → pastikan identitas firmware muncul.
5. Kirim config default → **Max torsi di aplikasi di-set 20–30% dulu**.
6. Centang **FFB aktif** → tekan tombol **Spring**: rim harus menarik ke
   tengah. Kalau malah mendorong keluar → centang **Invert motor**.
7. Aktifkan vJoy → buka `joy.cpl` (Run → `joy.cpl`) → pastikan `vJoy Device`
   ada dan sumbu X bergeser saat rim diputar. Kalau arahnya terbalik →
   **Invert encoder**.
8. Baru masuk game.

## 3. Alur data saat balapan

```
game: "constant force -3500"  ─┐
                               ▼
        vJoy callback ──> antrean event ──> FfbEngine (500 Hz)
                                                │ gaya total + gain + clamp
                                                ▼
                          serial: TORQUE -350 mPct ──> firmware ──> BTS7960
                                                ▲
game: sumbu X "vJoy Device" ◄── SetAxis ◄── telemetri sudut dari firmware
```

- Game umumnya **men-download 1 efek constant force** lalu meng-update
  magnitudenya setiap frame — itu gaya utama FFB.
- Spring/damper/friction/rumble datang sebagai efek terpisah dan dijumlahkan
  oleh engine (lihat `docs/04-protokol.md` untuk detail jenis efek).
- Saat tidak ada efek aktif, **auto-center idle** menjaga rim di tengah.

## 4. Setting game yang disarankan

- **Matikan/minimalkan "spring" dan "damper" bawaan game** (kecuali game
  memang mengandalkannya, mis. seri WRC) — setir DIY punya gesekan fisik
  sendiri; dobel-damper = lumpur.
- Set **rotasi game = rotasi di FfbBridge** (mis. 900° = 900°).
- Naikkan gain game sampai clipping (lihat flag `CLAMP`), lalu turunkan
  sedikit.
- "Min force" di FfbBridge menggantikan fitur min force wheel komersial.

## 5. File & lokasi

- Settings: `%APPDATA%\FfbBridge\settings.json`
- Log hanya tampil di jendela aplikasi (belum ada file log — roadmap).
