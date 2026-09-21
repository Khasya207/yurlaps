# Analisis hits rendah di track dan rencana receiver/protokol baru

Status: analisis teknik berdasarkan data lapangan; perlu capture dan pengujian terkontrol

Tanggal: 2026-09-21

## 1. Data lapangan

- Loop sekitar 3,5 m melintasi track.
- Jarak dua kabel loop dalam arah perjalanan sekitar 50 cm.
- Kawat loop 18 AWG.
- Kecepatan kendaraan diperkirakan 40–50 km/jam.
- Dua belas kendaraan berjalan bersama.
- Tiga kendaraan pernah missed lap dan transponder tersebut pernah menghasilkan hanya dua hits.
- Semua transponder memakai desain PCB, BOM, proses produksi, dan posisi pemasangan yang sama.
- Amplifier Cano memakai C1 39 pF tanpa jumper.

Kesimpulan awal: dua hits adalah margin digital yang terlalu kecil. Namun data sekarang belum dapat membedakan empat penyebab utama: transmitter lemah, pengaruh kendaraan/power, pola ID/learning, atau collision antarpaket.

## 2. Berapa paket yang sempat dikirim di atas loop?

Firmware transponder legacy mengirim telegram 96 bit pada sekitar 1,25 Mbit/s. Durasi data adalah:

```text
96 / 1.250.000 = 76,8 us
```

Dengan lead-in, lead-out, dan overhead, burst RF sekitar 80 us. Source publik transponder kemudian memberi delay tetap 2 ms ditambah urutan pseudo-random sekitar 0,1/0,2/0,4/0,8 ms. Perkiraan interval awal-paket ke awal-paket adalah sekitar 2,2–2,9 ms, rata-rata sekitar 2,48 ms.

Referensi source perilaku legacy:

- https://github.com/mv4wd/RCHourglass/blob/master/Firmware/Transponder/source/RCHourglassT_main.asm

### 2.1 Waktu melintasi panjang 50 cm

| Kecepatan | Kecepatan SI | Waktu untuk 0,50 m |
|---:|---:|---:|
| 40 km/jam | 11,11 m/s | 45,0 ms |
| 50 km/jam | 13,89 m/s | 36,0 ms |

### 2.2 Kesempatan paket teoritis

| Kecepatan | Dengan interval 2,2 ms | Rata-rata 2,48 ms | Dengan interval 2,9 ms |
|---:|---:|---:|---:|
| 40 km/jam | sekitar 20 | sekitar 18 | sekitar 15 |
| 50 km/jam | sekitar 16 | sekitar 14–15 | sekitar 12 |

Jadi jika receiver benar-benar dapat mendengar transponder sepanjang 50 cm, seharusnya tersedia kira-kira **12–20 kesempatan telegram**. Fase paket saat memasuki area dapat mengubah hasil sekitar satu paket.

Jarak kendaraan di antara awal dua telegram rata-rata adalah:

```text
40 km/jam: 11,11 m/s x 2,48 ms = 2,75 cm
50 km/jam: 13,89 m/s x 2,48 ms = 3,44 cm
```

Dua hits setara dengan area deteksi efektif hanya sekitar 5,5–6,9 cm bila packet cadence rata-rata dipakai. Namun angka 12–20 di atas adalah **upper bound dengan asumsi seluruh 50 cm merupakan area aktif kontinu**, bukan jumlah hits yang wajib terlihat.

Loop diferensial biasanya mempunyai dua lobe penerimaan di dekat kedua kabel dan null/pelemahan di tengah akibat pembatalan medan. Bila satu lobe yang efektif hanya selebar 5–10 cm, kendaraan hanya berada di lobe itu sekitar 3,6–9 ms dan hanya menyediakan sekitar 1–4 kesempatan paket. Decoder passage juga dapat menutup cluster pertama sebelum kendaraan mencapai kabel kedua. Dalam kondisi tersebut, dua hits dapat berasal dari dua packet valid di satu lobe, bukan bukti bahwa 10–18 packet lain semuanya rusak.

Jarak kabel 50 cm memberi waktu antarkabel sekitar 36–45 ms pada kecepatan ini. Sebagai pembanding, jarak referensi 30 cm memberi sekitar 21,6–27 ms. Memperlebar jarak kabel tidak otomatis menambah hits; ia dapat memisahkan dua lobe lebih jauh. Karena itu 30 cm dan 50 cm perlu diuji A/B pada lebar track, kendaraan, dan kecepatan yang sama.

Walaupun dua hits masih masuk akal pada model satu lobe, margin tersebut tetap terlalu kecil untuk target nol missed lap: hilangnya dua packet saja sudah menghilangkan passage.

## 3. Arti hits dan quality

- `hits` menunjukkan telegram yang diterima dan diterima decoder sebagai milik passage tersebut.
- Nilai `quality` firmware PSoC asli tidak dapat didefinisikan secara pasti karena source PSoC upstream tidak dipublikasikan.
- Interpretasi bahwa quality rendah berasal dari banyak packet yang tidak cocok dengan database/ID adalah masuk akal untuk mekanisme learning, tetapi belum dapat dipastikan dari source yang tersedia.
- Firmware YurLaps independen mendefinisikan quality berbeda, yaitu confidence voting sampel digital. Angkanya tidak boleh dibandingkan langsung dengan quality firmware asli atau RSSI MYLAPS.

Kombinasi dua hits, quality rendah, dan missed lap sudah cukup untuk menyatakan margin buruk walaupun arti numerik quality belum diketahui.

## 4. Mengapa tiga dari dua belas transponder dapat berbeda?

PCB dan BOM identik tidak menjamin margin RF identik. Perbedaan yang masih mungkin adalah:

- toleransi induktansi dan Q coil PCB;
- toleransi kapasitor tuning dan frekuensi resonansi;
- output drive PIC;
- frekuensi crystal/resonator;
- supply BEC, ripple, dan ground noise kendaraan;
- solder joint, flux/leakage, atau kerusakan mekanis;
- logam, carbon, kabel servo, dan harness di sekitar coil;
- pola bit ID tertentu menghasilkan jumlah/posisi phase reversal berbeda;
- packet collision ketika beberapa kendaraan melintas berdekatan.

MYLAPS yang bekerja di atas servo membuktikan margin sistem MYLAPS cukup besar pada instalasi itu. Hal tersebut tidak membuktikan bahwa transponder DIY dengan output, coil, tuning, dan receiver berbeda mempunyai margin yang sama.

## 5. Empat eksperimen yang memisahkan penyebab

### 5.1 Apakah masalah mengikuti PCB transponder atau kendaraan?

Tukar satu transponder buruk dengan satu transponder baik di antara dua kendaraan tanpa mengubah orientasi pemasangan.

| Hasil | Indikasi utama |
|---|---|
| masalah mengikuti PCB transponder | output RF, tuning coil, supply input PCB, atau ID |
| masalah tetap pada kendaraan | BEC/noise, material kendaraan, kabel, atau mounting |

### 5.2 Apakah masalah mengikuti hardware atau pola ID?

Setelah menyimpan firmware/ID asli, program ID transponder baik ke PCB yang lemah dan ID transponder lemah ke PCB yang baik. Jangan menjalankan dua ID duplikat bersamaan.

| Hasil | Indikasi utama |
|---|---|
| masalah mengikuti ID | pola telegram, learning/database, atau decoder |
| masalah mengikuti PCB | RF output/tuning hardware |

### 5.3 Apakah masalah berasal dari collision?

Jalankan setiap transponder lemah sendirian sebanyak 30–100 passage, lalu ulangi bersama dua belas kendaraan.

| Hasil | Indikasi utama |
|---|---|
| baik sendirian, buruk dalam rombongan | collision/capture effect sangat mungkin |
| tetap buruk sendirian | link analog, vehicle noise, tuning, atau ID |

Source legacy menginisialisasi pola pseudo-random delay secara sama pada setiap transponder, bukan dari ID unik. Perbedaan clock dan waktu power-up memberi decorrelation, tetapi persistent collision masih mungkin terjadi pada kendaraan berdekatan. Transponder yang lebih lemah juga cenderung kalah saat dua carrier 5 MHz overlap.

### 5.4 Apakah masalah berasal dari motor/ESC?

Untuk PCB dan kendaraan yang sama:

1. passage dengan motor/ESC tidak aktif;
2. passage dengan motor berputar tanpa beban;
3. passage pada kecepatan balap.

Penurunan hanya saat motor aktif menunjukkan EMI atau supply ripple, bukan kekurangan ID.

## 6. Log minimum yang diperlukan

Untuk setiap passage simpan:

```text
tanggal/waktu
ID transponder
PCB transponder
kendaraan
posisi kiri/tengah/kanan
sendiri atau rombongan
motor OFF/ON
kecepatan perkiraan
hits
quality
lap terdeteksi atau miss
jarak ke kendaraan terdekat
kondisi kering/basah
```

Tiga transponder yang selalu buruk berbeda diagnosisnya dari tiga transponder acak pada setiap race.

## 7. Apakah ID yang lebih pendek menyelesaikan masalah?

**Tidak dengan sendirinya.** Packet legacy 96 bit hanya memakai sekitar 76,8 us, sedangkan gap sekitar 2,1–2,8 ms. Memendekkan packet menjadi 64 bit tanpa mengubah gap menghemat sekitar 25,6 us, hanya sekitar satu persen dari interval total. Jumlah kesempatan paket hampir tidak berubah.

Mengurangi ruang ID terlalu jauh juga meningkatkan risiko ID duplikat dan false acceptance. Untuk produk komersial, minimum yang masuk akal adalah 16-bit ID, bukan hanya 4 atau 8 bit untuk dua belas mobil saat ini.

### 7.1 Kandidat protocol YurLaps baru

Kandidat awal 64-bit, belum merupakan keputusan final:

| Field | Bit |
|---|---:|
| sync/preamble | 16 |
| protocol version | 4 |
| unique ID | 16 |
| sequence/random slot | 4 |
| battery/status | 8 |
| CRC | 16 |
| total | 64 |

Pada 1,25 Mbit/s durasi datanya 51,2 us. Alternatif 72–80 bit dengan FEC harus dibandingkan karena mengoreksi satu atau dua bit mungkin lebih bermanfaat daripada menghemat 15–25 us.

Fitur yang lebih penting daripada sekadar memperpendek ID:

- CRC kuat agar packet rusak tidak menjadi ID palsu;
- sequence counter;
- pseudo-random backoff yang di-seed dari unique ID, sehingga dua transponder tidak mengulang slot yang sama;
- optional interleaving/FEC;
- transmitter output dan coil yang mempunyai production test limit;
- decoder dual-protocol yang menerima legacy dan protocol baru;
- RSSI/noise/coding-reject diagnostics pada receiver.

Satu receiver hard-limiter tidak dapat selalu memisahkan dua BPSK 5 MHz yang benar-benar overlap. Protocol baru dapat mengurangi kemungkinan collision, tetapi tidak menghapus near-far problem.

## 8. Apakah perlu membuat loop amplifier baru?

**Ya. Untuk target 5 m, 30 cm, dua belas kendaraan, dan nol missed lap, receiver baru lebih penting daripada hanya mengganti ID.** Protocol baru tidak dapat memperbaiki telegram yang bahkan tidak mencapai SNR minimum.

Prioritas receiver baru:

1. balanced input dengan impedansi dan damping terkontrol;
2. bandwidth cukup lebar untuk phase reversal, bukan resonansi sempit;
3. switchable gain atau gain yang dibekukan selama telegram;
4. limiter/comparator dengan threshold dan hysteresis terkontrol;
5. RSSI/envelope ke ADC;
6. background-noise dan overload measurement;
7. 75-ohm coax interface dan phantom power yang benar;
8. test points serta protection;
9. dukungan legacy active loop dan loop terminated baru;
10. production calibration dan self-test.

Amplifier baru juga tidak boleh langsung dianggap solusi bagi tiga transponder tertentu. Empat eksperimen pada bagian 5 harus dilakukan agar desain tidak mengobati penyebab yang salah.

## 9. Urutan pengembangan yang direkomendasikan

1. **Diagnosis lapangan:** swap PCB/kendaraan, swap ID, single-car vs pack, motor OFF/ON.
2. **Production test transponder:** fixture tetap untuk membandingkan carrier frequency, RF envelope, current, dan decoding pada tinggi standar.
3. **Receiver satu channel baru:** RSSI/noise + wideband phase path + legacy packet capture.
4. **Korelasikan:** RSSI, coding rejects, valid packets, hits, dan missed passage.
5. **Rancang protocol baru:** 16-bit ID minimum, CRC, ID-seeded backoff, kandidat FEC.
6. **Dual protocol PSoC:** legacy dan YurLaps baru berjalan bersamaan.
7. **Empat channel dan deduplication:** untuk track, sector, atau dua loop overlap.

Keputusan sekarang bukan memilih protocol *atau* amplifier. Jalur paling aman adalah memperbaiki observability dan receiver lebih dahulu, lalu protocol baru dirancang dari data error nyata.
