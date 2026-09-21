# Analisis tuning loop Cano/RCHourglass untuk track 4 m

Status: perhitungan teknik dan rencana uji; belum divalidasi pada loop fisik pengguna

Tanggal: 2026-09-21

## 1. Kesimpulan utama

Untuk loop satu lilitan berbentuk kira-kira **4,0 m x 0,30 m**, jawaban singkatnya adalah:

1. **Loop 4 m tidak boleh memakai setelan kapasitor loop pendek secara membabi buta.** Semakin panjang loop, umumnya semakin besar induktansinya dan semakin kecil kapasitansi yang diperlukan.
2. Spesifikasi lapangan Cano yang dipublikasikan hanya sampai **12 ft x 1 ft = 3,66 m x 0,305 m**. Setelan untuk 9–12 ft adalah **tanpa jumper**. Loop 4 m sama dengan 13,12 ft, jadi sudah sedikit melewati batas asli tersebut.
3. Untuk percobaan pertama 4 m, gunakan **C1 = 39 pF dengan J3, J4, dan J5 terbuka**. Jangan menambah C6/J4 atau menutup semua jumper.
4. Jika board sekarang disetel untuk loop sekitar 2,5 m dengan **J3 tertutup**, buka J3 ketika mencoba loop 4 m. Jika J5 tertutup, J5 juga harus dibuka.
5. Perhitungan geometri memberi induktansi sekitar **10,8–12,0 uH** dan kebutuhan kapasitansi efektif total sekitar **84–94 pF pada 5 MHz**. Ini adalah kapasitansi **total**, bukan nilai satu kapasitor yang harus disolder.
6. Selisih antara 39 pF yang terpasang dan 84–94 pF total dapat berasal dari kapasitansi loop, lead-in pasangan kabel, input transistor, PCB, konektor, dan tanah. Karena besarnya tidak diketahui, nilai akhir tidak dapat ditentukan dari lebar track saja.
7. Jika setelan tanpa jumper belum baik, lakukan uji A/B dengan data hits/miss. Kandidat berikutnya ditentukan oleh arah perubahan hasil, bukan dengan langsung menambahkan kapasitor terbesar.

Untuk target komersial **nol missed lap**, hasil hit bagus pada satu transponder diam belum cukup. Setelan harus lulus passage bergerak, motor/ESC aktif, beberapa posisi melintang track, kondisi kering/basah, dan kendaraan berdekatan.

## 2. Apa yang sebenarnya dilakukan rangkaian Cano

Loop dipasang langsung di antara dua rail input diferensial Q1/Q2. Jaringan yang terlihat pada `LoopAmplifierRevB6.jpg` adalah:

| Kondisi | Kapasitansi diskret nominal di antara rail | Beban tambahan |
|---|---:|---|
| Semua jumper terbuka | C1 = 39 pF | hanya beban input dasar |
| J3 tertutup | C1 + C5 = 39 + 22 = 61 pF | tidak ada resistor jumper tambahan |
| J4 tertutup | C1 + C6 = 39 + 39 = 78 pF | R12 = 4,87 kohm |
| J5 tertutup | C1 + C7 ~= 39 + 82 = 121 pF | R13 = 2,4 kohm |

C8 = 100 nF berada seri dengan cabang C7/R13. Pada 5 MHz reaktansinya hanya sekitar 0,32 ohm, sehingga untuk perhitungan tuning C7 tetap efektif sekitar 81,9 pF.

Jumper bukan sekadar memilih kapasitor. J4 dan J5 juga menambahkan resistor paralel yang mengubah impedansi input, Q, dan bandwidth. Penjelasan Howard Cano menyebut impedansi yang dilihat loop sekitar 1,3 kohm dan komponen jumper digunakan untuk memadatkan impedansi sekaligus mengubah tuning.

Build RCHourglass yang dipublikasikan secara eksplisit tidak memasang J4, C6, dan R12. Karena itu fungsi J4 tidak boleh diasumsikan sebagai setelan resmi untuk 4 m.

### Setelan ukuran yang dipublikasikan

Panduan yang dikutip kembali oleh maintainer RCHourglass adalah:

| Lebar loop dengan jarak kabel 1 ft | Setelan Cano |
|---|---|
| 9–12 ft = 2,74–3,66 m | tanpa jumper |
| 6–8 ft = 1,83–2,44 m | J3 |
| 3–4 ft = 0,91–1,22 m | J5 |

Tidak ada setelan resmi yang dipublikasikan untuk lebih dari 12 ft. Dengan demikian, **4 m bukan alasan untuk menutup jumper lain; justru setelan awalnya adalah semua jumper terbuka**.

## 3. Perhitungan induktansi

### 3.1 Asumsi

Perhitungan menggunakan loop persegi panjang satu lilitan:

- sisi panjang `a`: lebar track;
- sisi pendek `b`: jarak dua kabel dalam arah perjalanan, diasumsikan 0,30 m;
- kawat tembaga bulat;
- loop berada jauh dari logam dan belum memasukkan lead-in;
- induktansi frekuensi tinggi, sehingga kontribusi internal konduktor diabaikan.

Pendekatan Terman untuk satu lilitan persegi panjang digunakan. Setelah `L` didapat, kapasitansi resonansi ideal dihitung dengan:

```text
C = 1 / ((2 pi f)^2 L)
```

Pada 5 MHz, bila `L` dinyatakan dalam uH:

```text
C_total[pF] = 1013,21 / L[uH]
```

### 3.2 Hasil

| Ukuran loop | Kawat | L perkiraan | C total ideal @ 5 MHz |
|---|---|---:|---:|
| 2,5 m x 0,30 m | 24 AWG, diameter Cu 0,51 mm | 7,79 uH | 130,1 pF |
| 2,5 m x 0,30 m | 18 AWG, diameter Cu 1,02 mm | 7,01 uH | 144,6 pF |
| 3,5 m x 0,30 m | 24 AWG | 10,62 uH | 95,4 pF |
| 3,5 m x 0,30 m | 18 AWG | 9,56 uH | 106,0 pF |
| **4,0 m x 0,30 m** | **24 AWG** | **12,04 uH** | **84,2 pF** |
| **4,0 m x 0,30 m** | **18 AWG** | **10,84 uH** | **93,5 pF** |
| 5,0 m x 0,30 m | 24 AWG | 14,87 uH | 68,1 pF |
| 5,0 m x 0,30 m | 18 AWG | 13,40 uH | 75,6 pF |

Untuk 4 m, panjang konduktor persegi panjangnya sekitar:

```text
2 x (4,0 + 0,30) = 8,6 m
```

belum termasuk lead-in ke amplifier.

### 3.3 Sensitivitas geometri 4 m

Untuk kawat diameter tembaga sekitar 1 mm:

| Jarak dua kabel | L perkiraan | C total ideal |
|---:|---:|---:|
| 0,20 m | 9,96 uH | 101,7 pF |
| 0,30 m | 10,84 uH | 93,4 pF |
| 0,40 m | 11,55 uH | 87,7 pF |
| 0,50 m | 12,17 uH | 83,3 pF |

Jadi pernyataan "track 4 m" saja belum cukup untuk mendapatkan satu nilai kapasitor yang pasti.

## 4. Mengapa C total bukan C1 + jumper saja

Nilai yang perlu dipenuhi adalah:

```text
C_total = C1 + C_jumper + C_loop + C_lead-in + C_input + C_PCB + C_lingkungan
```

Maka:

```text
C_yang_dipasang = C_total_yang_diperlukan - semua_kapasitansi_parasitik
```

Rancangan referensi Cano memakai 24 AWG stranded zip speaker wire, bagian loop 1 ft x 8 ft, dan lead-in sekitar 3 ft yang kedua konduktornya tetap berdampingan. Lead-in semacam ini dapat menyumbang kapasitansi puluhan pF. Instalasi dengan amplifier tepat di tepi loop dan terminal sangat pendek akan mempunyai nilai berbeda.

Faktor lain:

- diameter tembaga, bukan hanya diameter luar isolasi;
- panjang dan jarak lead-in;
- apakah lead-in berupa zip cord, twisted pair, atau dua kabel terpisah;
- kapasitansi input transistor dan layout perfboard/PCB;
- loop ditanam sekitar 2 cm, kelembapan tanah, dan perubahan saat hujan;
- rebar, pagar, pipa, drain, pelat baja, atau kabel daya dekat loop;
- sambungan yang korosi atau kemasukan air;
- perimeter 8,6 m sudah sekitar 14% panjang gelombang 5 MHz di udara, sehingga model lumped ideal mulai kurang sempurna.

Coax 75 ohm **setelah loop amplifier** bukan bagian paralel langsung dari loop karena dipisahkan oleh amplifier. Panjang coax tidak boleh dikompensasikan dengan sembarang penambahan kapasitor loop. Jangan mengubah jaringan 75 ohm output/power-feed hanya karena loop diperpanjang.

## 5. Rekomendasi praktis untuk instalasi pengguna

### 5.1 Loop 3,5 m yang sekarang

Loop 3,5 m x sekitar 0,30 m masih masuk rentang Cano 9–12 ft. Setelan awal yang benar adalah:

```text
C1 = 39 pF
J3 = OPEN
J4 = OPEN
J5 = OPEN
```

Jika J3 atau J5 sekarang tertutup, itu kandidat penyebab detuning. Namun bila semua jumper sudah terbuka dan hits masih 3–4, jangan menyimpulkan kapasitor adalah satu-satunya penyebab. Periksa lead-in, sambungan, air, logam, posisi amplifier, noise motor/ESC, dan DC balance.

### 5.2 Loop 4 m

Urutan yang disarankan:

1. **Baseline resmi terdekat:** C1 = 39 pF, semua jumper terbuka.
2. Uji dengan loop dan lead-in persis dalam posisi permanen, bukan loop digulung di meja.
3. Jika hasil buruk, coba J3 secara sementara sebagai pembanding. J3 menambah 22 pF. Jika hasil membaik, instalasi mempunyai kapasitansi efektif lebih rendah daripada asumsi referensi; nilai tambahan 10–22 pF dapat disaring kemudian.
4. Jika J3 memperburuk hasil, kembalikan ke tanpa jumper lalu bandingkan C1 = 33 pF dan 27 pF. Untuk loop yang benar-benar sudah optimum pada 3,5 m dengan 39 pF, perubahan induktansi ke 4 m secara teoritis mengarah ke pengurangan sekitar 11–13 pF, sehingga 27 pF merupakan kandidat eksperimen, **bukan nilai final yang dijamin**.
5. Jangan memakai J5 untuk 4 m. Jangan memasang J4/C6/R12 atau menggabungkan jumper hanya berdasarkan jumlah pF; cabang itu juga mengubah damping dan tidak dipakai pada build RCHourglass yang dipublikasikan.
6. Gunakan kapasitor C0G/NP0 5% atau lebih baik, kaki sependek mungkin. Jangan gunakan X7R/Y5V untuk tuning ini.
7. Matikan daya dan lepas USB sebelum mengubah jumper atau menyolder.

Matriks screening yang sederhana:

| Kandidat | C diskret nominal | Tujuan |
|---|---:|---|
| C1 39 pF, tanpa jumper | 39 pF | baseline Cano untuk loop terbesar |
| C1 39 pF + J3 | 61 pF | mengetahui apakah instalasi kekurangan C efektif |
| C1 33 pF, tanpa jumper | 33 pF | langkah kecil ke arah C lebih rendah |
| C1 27 pF, tanpa jumper | 27 pF | kandidat jika 33 pF masih lebih baik daripada 39 pF |

Jangan mencoba semua keadaan sekaligus. Hasil setiap perubahan harus dicatat.

## 6. Tuning tanpa oscilloscope

### 6.1 Pemeriksaan listrik dasar

1. Foto dan catat setelan awal agar dapat dikembalikan.
2. Dengan daya mati dan loop terlepas dari amplifier, periksa continuity loop. Nilainya harus mendekati resistansi probe DMM dan tidak berubah ketika sambungan digerakkan.
3. Periksa tidak ada kebocoran ke shield/coax, tanah, atau struktur logam.
4. Dengan loop belum terhubung, hidupkan amplifier dan set VR1 agar tegangan DC di antara dua terminal loop sedekat mungkin ke 0 V, sesuai petunjuk build. Ini hanya balance DC, bukan tuning 5 MHz.

### 6.2 Uji passage, bukan hanya jarak statis

Untuk setiap konfigurasi kapasitor:

1. Gunakan transponder, baterai, mobil, arah gerak, dan kecepatan yang sama.
2. Lakukan sedikitnya 20 passage pada sisi kiri, tengah, dan kanan track.
3. Ulangi dengan motor/ESC aktif.
4. Catat untuk setiap passage:
   - missed passage;
   - hits;
   - quality/confidence yang dilaporkan decoder;
   - false/phantom ID.
5. Kandidat terbaik adalah yang mempunyai **0 miss dan nilai minimum hits terbaik**, bukan satu nilai hits maksimum.
6. Kandidat yang lolos screening harus diuji 100 passage per posisi, lalu kondisi kering/basah dan dua kendaraan berdekatan.

BPSK membawa data pada pembalikan fase 180 derajat. Tuning yang terlalu tajam dapat memberi sinus 5 MHz yang besar tetapi menahan energi fase lama saat pembalikan, sehingga decoder justru kehilangan data. Karena itu optimasi berdasarkan LED atau amplitudo statis saja tidak cukup.

### 6.3 Alat murah yang membantu

LCR meter yang cukup baik dapat mengukur induktansi loop sekitar 10 uH setelah loop dilepas dari amplifier dan kapasitor. Nilai ini memperbaiki perkiraan L, tetapi tetap tidak mengukur seluruh kapasitansi terdistribusi pada 5 MHz. NanoVNA dapat membantu selama pengembangan, tetapi fixture dan loading 50 ohm harus dipahami agar hasil tidak disalahartikan.

Untuk pengguna tanpa alat RF, statistik passage decoder adalah alat keputusan akhir yang paling aman.

## 7. Konsekuensi untuk target 5 m

Cano asli mempunyai batas yang dipublikasikan sekitar 3,66 m. Keberhasilan sesekali pada 4–5 m tidak cukup untuk menjanjikan nol missed lap. Untuk produk hingga 5 m, pilihan yang lebih kuat adalah:

- receiver baru berbandwidth cukup lebar dengan diagnostik RSSI/noise;
- loop terminated/passive yang karakteristiknya lebih stabil;
- atau dua loop lebih pendek yang overlap pada dua channel, kemudian passage diduplikasi di firmware.

Modifikasi C1 pada Cano tetap berguna sebagai eksperimen cepat, tetapi bukan pengganti validasi receiver 5 m.

## 8. Data instalasi yang masih diperlukan

Nilai akhir baru dapat dipersempit setelah diketahui:

- jarak dua kabel loop yang sebenarnya;
- tipe dan luas penampang kawat, termasuk diameter tembaga;
- panjang total kawat;
- panjang, tipe, dan susunan lead-in dari loop ke amplifier;
- setelan J3/J4/J5 dan komponen yang benar-benar terpasang saat ini;
- jarak amplifier dari tepi loop;
- keberadaan rebar/logam/kabel daya;
- hits dan miss untuk masing-masing konfigurasi pada kondisi kering dan basah.

## 9. Sumber publik

- Skema Loop Amplifier Rev B6: https://github.com/mv4wd/RCHourglass/blob/master/Schematic/Decoder/LoopAmplifierRevB6.jpg
- Petunjuk build RCHourglass: https://github.com/mv4wd/RCHourglass/wiki/DecoderBuild
- Diskusi spesifikasi jumper dan ukuran Cano: https://www.rctech.net/forum/radio-electronics/1002584-rchourglass-diy-lap-timing-aka-cano-revised-66.html
- Penjelasan rancangan amplifier dan loop referensi: https://www.rctech.net/forum/radio-electronics/688671-lap-timing-decoder-3.html
- Penjelasan impedansi input dan fungsi padding jumper: https://www.rctech.net/forum/radio-electronics/688671-lap-timing-decoder-27.html
- Pentingnya bandwidth pada pembalikan fase: https://www.rctech.net/forum/radio-electronics/1002584-rchourglass-diy-lap-timing-aka-cano-revised-23.html
- Formula induktansi loop persegi panjang: https://www.qsl.net/in3otd/rlsim.html
