# ULR-RSSI-1 — receiver/diagnostic satu channel

Status: desain engineering Rev A; **belum PCB-ready dan belum tervalidasi di hardware**

Tanggal: 2026-09-21

## 1. Keputusan arsitektur

ESP32 tidak ditempatkan di box loop/amplifier yang menerima sinyal sangat kecil. Clock digital, regulator switching, SPI, dan Wi-Fi dapat menaikkan noise floor serta masuk kembali ke input ber-gain tinggi.

Arsitektur yang dipilih:

```text
Track                                      Decoder box

loop -> Cano active head -> 75R coax -> existing decoder phase path -> PSoC
                                           |
                                           +-> high-Z RSSI tap -> AD8309 -> PSoC SAR ADC
                                                                        |
                                                       packet/RSSI diagnostics
                                                                        |
                                                                      UART
                                                                        |
                                                              ESP32-C3 web UI
```

- Rev A adalah **monitor add-on nonintrusif**. Jalur decode Cano lama tetap dipakai, sehingga risiko merusak sistem race yang sudah bekerja kecil.
- PSoC mengambil RSSI agar sampling tepat terhadap telegram dan tidak terganggu ADC/Wi-Fi ESP32.
- ESP32 yang sudah ada hanya menyimpan, mengorelasikan, dan menampilkan web dashboard.
- Rev B nanti mengganti phase path lama dengan AD8338 + AD8309 limiter + comparator. Rev B baru dilakukan setelah Rev A memberi level input, noise, dan data reject nyata.

Ini lebih baik daripada menaruh ESP32 langsung di remote loop amplifier.

## 2. Mengapa AD8309

AD8309 dipilih untuk Rev A karena:

- status produk masih `PRODUCTION` pada 2026;
- usable 5–500 MHz, sehingga carrier 5 MHz berada di batas bawah spesifikasi tetapi tercakup;
- RSSI dynamic range nominal 100 dB;
- slope nominal 20 mV/dB;
- output VLOG sekitar 0,4–2,3 V, cocok untuk SAR ADC PSoC;
- mempunyai limiter differential yang dapat dipakai pada Rev B;
- input differential sekitar 1 kohm paralel 2,5 pF.

Datasheet:

- https://www.analog.com/media/en/technical-documentation/data-sheets/AD8309.pdf

AD8309 sangat broadband dan mudah menangkap noise digital. Karena itu layout, shielding, ground, dan pemisahan ESP32 merupakan bagian wajib dari desain, bukan aksesori.

## 3. Rev A: RSSI monitor aman

### 3.1 Pemasangan

Board monitor dipasang inline dekat decoder:

```text
coax dari Cano -> J1 RF_IN
J2 RF_THRU     -> konektor input decoder lama
```

Center J1 dan J2 tersambung langsung dengan trace pendek dan lebar. Decoder lama tetap menyediakan feed +5 V melalui R1 75 ohm dan tetap menjadi terminasi line.

Cabang monitor memakai R 2,21 kohm sehingga beban tambahan yang terlihat line kira-kira 3,21 kohm. Jika diparalel dengan terminasi 75 ohm, hasilnya sekitar 73,3 ohm. Perubahan loading hanya sekitar 2,2% dan harus diverifikasi pada bench sebelum race.

### 3.2 Jalur RSSI

```text
RF_THRU center -- 2.21k -- 10n C0G -- INHI AD8309
AGND -------------------- 10n C0G -- INLO AD8309
```

- 2,21 kohm menjaga tap berimpedansi tinggi.
- Attenuation tap dikalibrasi; jangan memakai rumus dBm datasheet secara langsung sebelum kalibrasi.
- Coupling capacitors memblokir DC phantom supply pada coax.
- Limiter AD8309 dinonaktifkan pada Rev A: RLIM tidak dipasang dan LMHI/LMLO diikat ke VPS2 sesuai datasheet.
- Kapasitor 33 pF antara FLTR dan VLOG memberi RSSI bandwidth sekitar 350 kHz. Ini cukup cepat untuk melihat burst sekitar 80 us, tetapi menurunkan ripple 10 MHz dari full-wave detector.

### 3.3 Power

- Input 5 V bersih dari decoder box, bukan dari center coax.
- LP5907-3.3 low-noise LDO untuk rail analog.
- VPS1 dan VPS2 masing-masing diberi isolasi 10 ohm, 100 nF, dan 1 uF sedekat mungkin ke pin.
- COM1, COM2, dan PADL mengikuti rekomendasi datasheet dan terhubung ke analog ground plane dengan via pendek.

### 3.4 RSSI ke PSoC

Koneksi provisional:

```text
AD8309 VLOG -> 47R -> RSSI_OUT -> P3[0] PSoC
                             |
                            1nF
                             |
                            AGND
```

P3[0] dipilih karena accessible dan tidak mempunyai bypass capacitor 1 uF bawaan seperti P3[2]. Routing analog PSoC Creator tetap harus dikonfirmasi saat project dibuat.

Target awal SAR ADC:

- 12 bit;
- 100 ksample/s;
- input 0–Vdda;
- DMA ring;
- baseline noise median sebelum burst;
- peak, mean, dan integral RSSI per capture window.

## 4. Data yang harus dihasilkan

Untuk setiap telegram/passage, firmware diagnostic harus memiliki data internal:

```text
channel
timestamp
noise_adc
peak_adc
noise_dbm_calibrated
peak_dbm_calibrated
delta_db
preamble_candidates
coding_rejects
valid_packets
id
hits
digital_quality
overload
```

ESP32 menerima frame diagnostic dan mengorelasikannya menjadi:

- grafik RSSI terhadap waktu;
- dua lobe kabel loop;
- noise sebelum kendaraan datang;
- peak dan minimum margin per transponder;
- valid/reject ratio;
- perbandingan motor OFF/ON;
- perbandingan single-car/12-car;
- daftar transponder yang konsisten lemah.

## 5. Kalibrasi RSSI

Rumus nominal AD8309 untuk sistem 50 ohm adalah kira-kira:

```text
Pin[dBm] = VLOG[V] / 0,020 - 95
```

Rumus ini **tidak boleh langsung dipakai sebagai hasil absolut** karena Rev A memakai:

- line 75 ohm;
- tap 2,21 kohm;
- coupling/layout loss;
- carrier tepat di batas bawah 5 MHz;
- toleransi slope/intercept IC.

Kalibrasi minimum dua titik diperlukan dengan generator 5 MHz atau fixture transmitter referensi. Sebelum alat tersedia, gunakan:

```text
delta_dB = (Vpeak - Vnoise) / 0,020
```

Nilai delta relatif jauh lebih berguna daripada dBm palsu.

## 6. Rev B: receiver pengganti

Blok kandidat setelah data Rev A tersedia:

```text
legacy active / passive loop input
               |
      selectable feed + termination
               |
     protection + wide 3–8 MHz filtering
               |
       AD8338 differential VGA
       gain fixed during telegram
               |
       AD8309 RSSI + limiter
          |               |
        VLOG          LMHI/LMLO
          |               |
      PSoC SAR       TLV3601 comparator
          |               |
          +------- PSoC digital capture
```

Kandidat:

- AD8338: LF–18 MHz, fully differential, 0–80 dB controlled gain;
- AD8309: RSSI + phase-preserving limiter;
- TLV3601: rail-to-rail 2,5 ns differential comparator ke CMOS.

AGC tidak boleh bergerak cepat di tengah telegram. Rev B akan memakai controlled gain yang dipilih dari noise scan lalu dibekukan selama burst.

Referensi:

- https://www.analog.com/media/en/technical-documentation/data-sheets/AD8338.pdf
- https://www.ti.com/product/TLV3601

## 7. ESP32

ESP32-C3 YurLaps yang sudah ada tetap dipakai untuk:

- UART dari PSoC;
- penyimpanan statistik;
- web dashboard;
- CSV export;
- korelasi ID/hits/quality/RSSI/miss.

Aturan EMI:

- board ESP32 dipisah dari analog board dengan kabel UART/GND pendek atau digital isolator pada revisi akhir;
- antenna ESP32 berada jauh dari input coax dan AD8309;
- analog board memakai shield can;
- regulator switching ESP32 tidak boleh memberi supply langsung ke rail analog;
- uji Wi-Fi OFF versus ON wajib masuk validation matrix.

## 8. Yang belum boleh dilakukan

- Jangan memasang board Rev A pada race sebelum pass-through, phantom power, dan loading diuji.
- Jangan menghubungkan VLOG langsung ke GPIO digital.
- Jangan menaruh ESP32/Wi-Fi dalam box remote Cano.
- Jangan menyatakan angka dBm akurat tanpa kalibrasi.
- Jangan mengganti phase path lama dengan limiter Rev B sebelum waveform dan packet-error test selesai.
- Jangan memesan PCB produksi dari dokumen ini; schematic CAD, layout, DRC, dan prototype bench masih harus dibuat.

## 9. Tahap implementasi

1. Rev A schematic CAD dan PCB 2-layer dengan shield footprint.
2. Assemble tiga board RSSI monitor.
3. Bench verify pass-through dan DC feed tanpa transponder.
4. Kalibrasi relatif dengan satu transponder referensi.
5. Tambahkan SAR ADC/DMA pada firmware diagnostic PSoC terpisah.
6. Tambahkan parser dan dashboard RSSI ke ESP32-C3.
7. Uji 12 transponder, swap car/ID, 30 cm/50 cm, motor OFF/ON.
8. Gunakan data tersebut untuk menentukan gain/filter Rev B.
9. Baru buat Rev B receiver pengganti.
