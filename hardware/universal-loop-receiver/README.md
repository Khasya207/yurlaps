# YurLaps Universal Loop Receiver (ULR)

Status: arsitektur awal / belum menjadi skema produksi

Tanggal: 2026-09-16

Analisis khusus jaringan tuning Cano untuk loop 3,5–4 m tersedia di
[CANO-LOOP-TUNING-4M-ID.md](CANO-LOOP-TUNING-4M-ID.md).

## 1. Sasaran produk yang disepakati

- Carrier legacy: 5 MHz; decoder baru harus kompatibel dengan transponder RCHourglass yang sudah digunakan.
- Panjang loop per channel: sekitar 1 m sampai maksimum 5 m melintasi track.
- Jarak antarkabel loop arah perjalanan: nominal 30 cm; 30–40 cm harus diuji.
- Kedalaman loop: maksimum 2 cm untuk instalasi RC permanen.
- Jarak vertikal transponder yang ditargetkan: 30 cm pada kecepatan balap.
- Coax: 75 ohm, maksimum 15 m per channel untuk spesifikasi awal.
- Empat receiver channel paralel dengan satu timebase PSoC.
- Mendukung dua antarmuka loop:
  1. `LEGACY_ACTIVE`: loop sederhana dengan loop amplifier Cano/RCHourglass yang diberi daya melalui coax.
  2. `PRO_PASSIVE`: loop terminated dengan passive matching box di track dan receiver aktif di decoder.
- Konfigurasi channel: track independen, sector, atau logical/redundant group.

Target 30 cm lebih berat daripada panduan MYLAPS RC lama yang menyebut maksimal 15 cm. Karena itu 30 cm adalah target pengembangan yang harus divalidasi, bukan klaim produk sebelum pengujian selesai.

## 2. Pelajaran dari dokumen publik MYLAPS

Dokumen publik MYLAPS RC menunjukkan bahwa reliabilitas berasal dari keseluruhan sistem, bukan satu amplifier:

- Dua kabel loop sejajar nominal 30 cm.
- Track maksimum yang disebut manual lama adalah 10 m.
- Slot loop maksimum 2 cm.
- Coax 75 ohm double-shield hingga 100 m pada manual tersebut.
- Sistem mengukur background noise, signal strength, dan hits.
- Target manual: noise 0–40, strength sedikitnya 100, dan hits sedikitnya 20 pada kecepatan; skala strength tersebut milik MYLAPS dan tidak boleh dianggap sama dengan quality RCHourglass.
- Pemasangan transponder RC disarankan tidak lebih tinggi dari 15 cm dan tanpa metal/carbon di antara transponder dan loop.

Referensi:

- https://www.transponderservices.com/docs/MYLAPS_RC_Manual_vJan2010.pdf
- https://www.mxtransponder.com/MYLAPS_Decoder_Manual_vJan2010.pdf

Dokumen resmi tidak membuka skema receiver. Rekonstruksi komunitas atas connection box lama memperlihatkan pendekatan loop terminated, transformer/balun, coupling capacitors, coax signature resistor, dan tanpa amplifier aktif di box. Itu bukan bukti bahwa semua produk MYLAPS memakai skema yang persis sama.

- https://github.com/condac/openAST/tree/master/circuitDesign/amb_blackbox

Konsep teknis yang dapat diambil secara independen:

1. Stabilkan impedansi dan Q loop dengan terminasi lebar pita.
2. Gunakan transformer untuk balanced-to-unbalanced conversion dan isolasi common-mode.
3. Kirim sinyal melalui coax berimpedansi terkendali.
4. Letakkan receiver kompleks, diagnostik, dan gain control di decoder yang kering serta mudah diservis.
5. Ukur noise dan signal margin per channel.
6. Timestamp setiap packet pada satu global clock.

Paten MYLAPS yang masih dapat aktif di sebagian yurisdiksi membahas penentuan crossing berdasarkan phase transition dan/atau signal strength. Implementasi komersial algoritma crossing perlu tinjauan IP profesional dan tidak boleh menyalin klaim paten secara langsung.

- https://patents.google.com/patent/US9460564B2/en
- https://patents.google.com/patent/EP3035298A1/en

## 3. Keputusan arsitektur

### 3.1 Bukan satu remote amplifier universal

Satu amplifier dengan gain tinggi bukan solusi terbaik untuk semua ukuran loop. Loop pendek dan transponder dekat dapat membuat receiver overload, sedangkan loop panjang dan transponder jauh memerlukan noise floor rendah. AGC yang bergerak tanpa kontrol juga dapat mengubah sinyal selama telegram.

Desain dibagi menjadi:

```text
PRO_PASSIVE track box             Central 4-channel receiver

terminated balanced loop         75R input + protection
        |                                  |
passive transformer/balun  --->  75R coax 15 m
                                           |
                                  wide low-Q filtering
                                           |
                                 VGA/log-limiter candidate
                                           |------> RSSI/envelope -> PSoC ADC
                                           `------> comparator ----> PSoC digital decoder
```

Untuk instalasi lama:

```text
LEGACY_ACTIVE Cano/RCH amp ---> 75R coax ---> receiver low-gain/bypass path
                   ^
                   `--- selectable +5 V feed from decoder
```

### 3.2 Dua jenis track box

#### ULR-P: passive professional-style box

- Loop berupa dua kabel sejajar, ditutup resistor di ujung yang berlawanan dengan box.
- Nilai terminasi prototype: 330 ohm dan 470 ohm, diuji keduanya.
- Balanced transformer/balun ke coax 75 ohm.
- Kandidat matching awal:
  - 330 ohm + transformer impedance ratio 1:4: 330/4 = 82.5 ohm, dekat 75 ohm.
  - 470 ohm + custom ratio sekitar 1:6.25: 470/6.25 = 75.2 ohm.
  - 470 ohm + 1:8 sebagai pembanding.
- Tidak menggunakan narrow resonant tuning yang sensitif terhadap panjang loop dan tanah.
- Resistor signature sekitar 100 kohm dapat digunakan untuk diagnostik open/short; nilai final harus disesuaikan dengan rangkaian monitor.
- ESD/surge protection berkapasitansi rendah.
- Box pasif, shielded, dan waterproof.

Kandidat transformer harus diuji dengan VNA/oscilloscope pada loop 1 m, 2.5 m, 3.5 m, dan 5 m. Rasio matematis saja tidak cukup karena impedansi loop bersifat kompleks dan berubah dengan pemasangan.

#### ULR-L: legacy active interface

- Menggunakan amplifier Cano/RCHourglass yang sudah ada.
- Receiver mengaktifkan feed +5 V melalui jaringan yang kompatibel dengan input lama.
- Gain tambahan di decoder dibypass atau disetel rendah.
- Current/open/short monitor melindungi tiap channel.

Tidak boleh menghubungkan ULR-P dan amplifier Cano secara seri.

## 4. Central receiver per channel

Blok yang direncanakan:

1. BNC/F connector 75 ohm.
2. Low-capacitance ESD/surge protection.
3. Coax DC monitor untuk mendeteksi open, short, passive signature, dan current active amplifier.
4. Selectable legacy +5 V feed.
5. 75 ohm termination dan AC coupling.
6. Low-Q band limit sekitar carrier; passband awal harus cukup lebar untuk menjaga sideband/perubahan fase BPSK, kira-kira 3–8 MHz sebagai titik simulasi, bukan nilai final.
7. Bypassable low-noise preamp untuk passive mode.
8. High-dynamic-range gain/limiting stage.
9. RSSI/envelope output ke ADC PSoC.
10. Differential comparator/limiter output ke digital fabric PSoC.
11. Analog test point untuk oscilloscope dan production test.

### Kandidat A: AD8338

Alasan:

- Fully differential signal path.
- Operasi LF sampai 18 MHz, sehingga carrier dan sideband 5 MHz tidak berada tepat di batas bawah IC.
- Gain control nominal 0–80 dB.
- Fitur AGC/detector dan offset correction.
- Ditujukan antara lain untuk inductive telemetry.

Rencana penggunaan yang disukai adalah controlled gain dengan gain dibekukan selama telegram, atau AGC yang dibuktikan tidak merusak fase/symbol. Output differential masih memerlukan comparator/receiver stage.

Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/AD8338.pdf

### Kandidat B: AD8309

Alasan:

- Log-limiting receiver dengan RSSI sekitar 100 dB dynamic range.
- Limiter output dan RSSI dalam satu IC.
- Data tersedia pada 5 MHz.

Risiko:

- 5 MHz berada di batas bawah spesifikasi; fidelity sideband di bawah carrier harus diukur.
- Limiter output memerlukan interface differential yang benar.

Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/AD8309.pdf

Dua candidate front-end harus dibandingkan. Pemilihan final tidak boleh dilakukan hanya dari datasheet.

## 5. Mengapa passive box layak untuk coax 15 m

Contoh RG6 berkualitas memiliki attenuation sekitar 1.9 dB/100 m pada 5 MHz. Pada 15 m, loss kabel hanya sekitar 0.285 dB. Dengan demikian, untuk target awal 15 m, active amplifier di track tidak diperlukan hanya untuk mengatasi cable loss. Matching, common-mode rejection, receiver noise, connector, dan instalasi lebih penting.

Referensi contoh kabel: https://www.pasternack.com/images/ProductPDF/RG6-CATV.pdf

## 6. Telemetri diagnostik per channel

Receiver harus melaporkan sekurangnya:

```text
channel mode
coax status: OK / OPEN / SHORT / UNKNOWN
active amplifier current
background noise
packet RSSI or envelope
valid packets
invalid preambles
CRC/FEC result where applicable
hits per passage
quality/confidence
clipping/overload indication
selected gain
```

Tujuannya agar instalasi bisa disetel tanpa oscilloscope, walaupun oscilloscope tetap diperlukan selama pengembangan analog.

## 7. Prinsip firmware

- Capture empat channel secara paralel; jangan memakai interrupt CPU per edge 5 MHz.
- Semua channel memakai satu global hardware timer.
- Setiap packet mendapat timestamp, channel, RSSI/quality, dan decode status.
- Gain tidak boleh berubah sembarangan di tengah telegram.
- Passage dibentuk dari cluster beberapa packet dengan ID konsisten.
- Logical group melakukan deduplication jika satu transponder terbaca dua loop.
- ESP32/YurLaps harus memakai timestamp PSoC dan channel/group, bukan `millis()` saat UART selesai menerima record.

Metode final penentuan crossing time harus dikaji terhadap paten aktif dan dirancang secara independen.

## 8. Tahap prototype

### P0 — baseline

- Original Cano amplifier dan firmware asli.
- Loop 2.5 m dan 3.5 m, surface/buried.
- Simpan monitor, hits, quality, false reads.

### P1 — passive box comparison

Buat tiga passive matching prototype:

- P1-A: 330 ohm + 1:4 transformer.
- P1-B: 470 ohm + custom sekitar 1:6.25.
- P1-C: 470 ohm + 1:8 transformer.

Uji dengan receiver laboratorium/SDR dan kemudian receiver analog candidate.

### P2 — one-channel receiver evaluation

- Satu channel AD8338 evaluation.
- Satu channel AD8309 evaluation.
- Comparator, RSSI ADC, input monitor, dan test point.
- PSoC diagnostic firmware.

### P3 — one-channel decode

- Decode satu legacy RCHourglass transponder.
- Static, moving, motor OFF/ON.
- Loop 1, 2.5, 3.5, dan 5 m.

### P4 — four channels

- Empat receiver paralel.
- Channel isolation dan simultaneous packet tests.
- Independent, sector, adjacent, dan redundant groups.

## 9. Kriteria validasi

Screening tiap kombinasi loop/mode:

- 20 passage kiri, tengah, kanan: 0 miss.
- Dilanjutkan 100 passage per posisi: 0 miss dan 0 false.
- Motor/ESC OFF dan ON.
- Beberapa transponder lemah dan kuat.
- Dua sampai empat kendaraan berdekatan.
- Track kering dan basah.

Kualifikasi desain:

- Sedikitnya 1,000 passage pada kondisi kritis.
- 0 missed passage dan 0 phantom ID pada dataset kualifikasi.
- Minimum packet margin ditentukan setelah korelasi RSSI/hits dengan hasil lap.
- Loop 1 m, 2.5 m, 3.5 m, dan 5 m.
- Coax 1 m, 5 m, 15 m; 25 m sebagai eksperimen tambahan.
- Temperatur dan supply corner tests.
- ESD, short coax, open loop, dan power cycling.

Tidak ada klaim `30 cm guaranteed` sebelum seluruh pengujian di atas lulus dengan beberapa jenis transponder.

## 10. Keputusan yang masih harus dibuktikan

- Nilai terminasi 330 atau 470 ohm.
- Transformer ratio dan core final.
- AD8338 vs AD8309.
- Band-limit network dan group-delay target.
- Comparator/interface final.
- Gain strategy dan freeze timing.
- Kekuatan legacy transponder pada target 30 cm.
- Apakah target 30 cm membutuhkan transponder YurLaps baru yang lebih kuat.
- Implementasi timing yang tidak melanggar paten aktif pada pasar tujuan.
