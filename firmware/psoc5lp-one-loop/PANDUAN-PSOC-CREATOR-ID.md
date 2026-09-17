# Panduan sangat rinci: membuat firmware decoder satu loop di PSoC Creator 4.4

Panduan ini ditujukan untuk **CY8CKIT-059 dengan PSoC 5LP
CY8C5888LTI-LP097** dan PCB input decoder RCHourglass/Cano yang sudah ada.
Ikuti checkpoint secara berurutan. Jangan langsung memprogram decoder yang
sedang dipakai untuk balapan.

## Status project dan batas pengujian

Source C yang digunakan ada di folder:

```text
firmware\psoc5lp-one-loop\OneLoopDecoder.cydsn\
```

Codec, demodulator sintetis, format UART, passage tracker, timebase wrap, dan
state wrapper DMA sudah lulus host test. Tetapi `TopDesign.cysch` hanya dapat
dibuat oleh PSoC Creator di Windows dan capture DMA 20 MS/s belum diuji pada
board/loop fisik. Karena itu percobaan pertama harus dianggap sebagai prototype,
bukan firmware balapan yang sudah tervalidasi.

---

# Bagian A — persiapan dan keamanan

## A1. Peralatan

Siapkan:

1. Komputer Windows 10 atau Windows 11.
2. CY8CKIT-059, sebaiknya satu board cadangan.
3. Kabel USB atau port USB untuk konektor KitProg di ujung board.
4. Source repository YurLaps ini di folder lokal, misalnya:
   `C:\YurLaps\yurlaps`.
5. Satu transponder RCHourglass yang ID-nya diketahui.
6. Loop dan amplifier Cano yang saat ini terbukti bekerja.
7. Program terminal, misalnya Tera Term atau PuTTY.
8. Salinan HEX RCHourglass yang saat ini bekerja untuk rollback.

## A2. Kenali dua konektor USB CY8CKIT-059

CY8CKIT-059 memiliki bagian KitProg/programmer dan bagian target PSoC 5LP.
Gunakan konektor/PCB USB pada sisi **KitProg** untuk:

- SWD programming;
- debugging;
- KitProg USB-UART.

Konektor target USB di sisi PSoC bukan jalur programming firmware ini. Firmware
satu-loop pertama juga belum membuat USB CDC pada konektor target tersebut.

## A3. Jangan hapus decoder kerja tanpa rollback

Sebelum flashing:

1. Simpan HEX firmware lama.
2. Catat versi HEX yang sedang digunakan.
3. Foto sambungan P12[2], P12[3], P12[6], P12[7], 5 V, dan GND.
4. Catat posisi/orientasi CY8CKIT-059 pada socket PCB decoder.
5. Untuk percobaan pertama, putuskan coax/amplifier dan supply eksternal.
6. Jangan memberi daya dari dua sumber 5 V sekaligus kecuali rangkaiannya sudah
   dipastikan aman.

**Checkpoint A:** HEX lama tersedia dan wiring sudah difoto. Jika belum, jangan
melanjutkan ke flashing.

---

# Bagian B — instalasi PSoC Creator

## B1. Program yang diperlukan

Pasang:

1. **PSoC Creator 4.4**;
2. PSoC 3/4/5 device support;
3. **PSoC Programmer**;
4. driver KitProg;
5. driver KitProg USB-UART;
6. toolchain ARM GCC yang disertakan oleh Creator.

PSoC Creator berbeda dari ModusToolbox. Project ini memang memakai Creator 4.4
karena CY8CKIT-059/PSoC 5LP menggunakan schematic programmable fabric Creator.

Board resmi:

<https://www.infineon.com/evaluation-board/CY8CKIT-059>

## B2. Periksa driver

1. Hubungkan sisi KitProg CY8CKIT-059 ke komputer.
2. Buka **Device Manager**.
3. Cari dua perangkat:
   - KitProg atau KitProg2;
   - `KitProg USB-UART (COMxx)`.
4. Catat nomor COM, misalnya `COM7`.
5. Buka PSoC Programmer sekali.
6. Jika ada permintaan update firmware KitProg, lakukan update lalu cabut dan
   pasang ulang board.

Jika perangkat tidak dikenal, driver biasanya ada di sekitar:

```text
C:\Program Files (x86)\Cypress\Programmer\drivers\KitProg\
```

**Checkpoint B:** KitProg terlihat dan satu nomor COM tersedia.

---

# Bagian C — membuat workspace dan project kosong

## C1. Jalankan wizard

1. Buka **PSoC Creator 4.4**.
2. Pilih **File > New > Project...**.
3. Pilih jenis project PSoC 5LP atau target device.
4. Pada device selector, cari dan pilih persis:

```text
CY8C5888LTI-LP097
```

5. Jika pilihan kit tersedia, `CY8CKIT-059` boleh dipilih, tetapi tetap periksa
   bahwa device akhirnya `CY8C5888LTI-LP097`.
6. Pilih template **Empty schematic**.
7. Isi project name:

```text
OneLoopDecoder
```

8. Isi workspace name:

```text
OneLoopDecoderWorkspace
```

9. Gunakan lokasi tanpa sinkronisasi OneDrive dan sebaiknya tanpa karakter
   khusus, contohnya:

```text
C:\YurLaps\OneLoopDecoderWorkspace
```

10. Klik **Finish**.

Hasilnya kurang lebih:

```text
C:\YurLaps\OneLoopDecoderWorkspace\
  OneLoopDecoderWorkspace.cywrk
  OneLoopDecoder.cydsn\
    OneLoopDecoder.cyprj
    OneLoopDecoder.cydwr
    main.c
    TopDesign\TopDesign.cysch
```

## C2. Pilih compiler

1. Klik kanan project `OneLoopDecoder` pada Workspace Explorer.
2. Pilih **Build Settings...**.
3. Pastikan toolchain/compiler adalah **ARM GCC** bawaan Creator.
4. Jangan memilih compiler 8051; itu bukan untuk PSoC 5LP Cortex-M3.
5. Untuk build pertama gunakan konfigurasi `Debug` atau default.
6. Optimization default boleh dipakai. Setelah berfungsi, Release dapat memakai
   `-O2`.

ARM GCC penting karena capture buffer diberi alignment 4096 byte agar tidak
melewati batas alamat atas DMA 64 KiB.

**Checkpoint C:** title project menunjukkan `OneLoopDecoder` dan device
`CY8C5888LTI-LP097`.

---

# Bagian D — memahami jalur input sebelum menggambar TopDesign

Untuk PCB decoder asli, tidak ada Comparator component di TopDesign.

```text
coax dari amplifier
 -> C1
 -> transistor Q1-Q4/input conditioner
 -> C2 dan bias R9/R10
 -> P12[2] digital input
```

P12[2] sudah menerima sinyal yang diperkuat/dibatasi oleh rangkaian transistor.
Input buffer digital P12[2] menjadi decision threshold. P12[3] mengikuti level
P12[2] dan memberi positive feedback melalui R11 = 4.87 kOhm:

```text
P12[2] -> fabric PSoC -> P12[3] -> R11 4.87 kOhm -> node P12[2]
```

Jangan sambungkan coax mentah ke P12[2]. Jika suatu saat rangkaian Q1-Q4 tidak
dipakai atau digunakan passive loop receiver, pasang external high-speed
comparator/limiter sebelum P12[2].

---

# Bagian E — menambahkan komponen TopDesign

Buka `TopDesign.cysch` dengan double-click. Tampilkan **Component Catalog** jika
belum terlihat melalui menu **View > Component Catalog**.

Nama instance di bawah ini **harus persis sama**, termasuk huruf besar/kecil,
karena source C memanggil API dengan nama tersebut.

## E1. `LoopIn`

1. Cari komponen **Pins** atau **Digital Input Pin**.
2. Drag ke TopDesign.
3. Ubah instance name menjadi:

```text
LoopIn
```

4. Double-click komponen.
5. Atur:
   - Number of pins: `1`;
   - Type: `Digital Input`;
   - HW connection: dicentang;
   - Drive mode: `High Impedance Digital`;
   - Input buffer: enabled;
   - Input synchronization: `Transparent` atau `None/Unsychronized`;
   - Interrupt: `Rising Edge`.
6. Klik **OK**.

Setelah interrupt diaktifkan, simbol harus memiliki terminal sinyal digital dan
terminal interrupt.

Jangan memilih `Analog`, karena DMA membaca Port Status register digital.

## E2. `HystOut`

1. Tambahkan **Digital Output Pin**.
2. Nama instance:

```text
HystOut
```

3. Atur:
   - Number of pins: `1`;
   - Digital Output;
   - HW connection: dicentang;
   - Drive mode: `Strong Drive`;
   - Slew rate: `Fast`;
   - Initial value boleh `0`; setelah fabric aktif nilainya mengikuti LoopIn.
4. Klik **OK**.

HW connection harus aktif agar komponen mempunyai terminal input dari fabric.

## E3. `LoopEdgeISR`

1. Cari komponen **Interrupt** di bagian System.
2. Drag ke TopDesign.
3. Nama instance:

```text
LoopEdgeISR
```

4. Tipe interrupt: `Derived`/default.
5. Priority default boleh dipakai untuk percobaan pertama.

ISR hanya menerima edge pertama. Firmware kemudian menonaktifkan interrupt edge
selama window capture, sehingga CPU tidak menerima lima juta interrupt/detik.

## E4. `SampleClock`

1. Tambahkan komponen **Clock**.
2. Nama instance:

```text
SampleClock
```

3. Set desired frequency:

```text
20 MHz
```

4. Clock source harus berasal dari BUS_CLK 80 MHz dengan divider 4.
5. Jika ada `Start automatically`, nonaktifkan; source memanggil
   `SampleClock_Start()` dan `SampleClock_Stop()`.
6. Duty cycle 50%.

## E5. `SampleDMA`

1. Cari komponen **DMA** di bagian System.
2. Drag ke TopDesign.
3. Nama instance:

```text
SampleDMA
```

4. Double-click dan atur:
   - Hardware Request (`drq`): enabled;
   - Request type: **Rising Edge**;
   - Hardware Termination (`trq`): disabled;
   - Termination output (`nrq`): disabled, kecuali Creator menampilkan terminal
     yang tidak digunakan;
   - Priority: gunakan prioritas tinggi/default yang tidak dipakai channel lain.
5. Klik **OK**.

Request harus rising-edge. Jika disetel level, DMA dapat mentransfer jumlah byte
yang salah selama clock berada pada level high.

Burst count dan request-per-burst tidak diisi di schematic; source
`capture_dma.c` menginisialisasinya sebagai satu byte per request.

## E6. `HostUART`

1. Cari komponen **UART** di bagian Communications.
2. Drag ke TopDesign.
3. Nama instance:

```text
HostUART
```

4. Atur:
   - Mode: `Full UART`;
   - TX: enabled;
   - RX: enabled;
   - Clock: Internal;
   - Baud rate: `57600`;
   - Data bits: `8`;
   - Parity: `None`;
   - Stop bits: `1`;
   - Flow control: `None`;
   - RX buffer: `64` bytes;
   - TX buffer: `64` bytes.
5. Periksa actual baud error. Nilainya harus kecil karena clock 80 MHz.

## E7. `HostRx`

1. Tambahkan satu Digital Input Pin.
2. Nama:

```text
HostRx
```

3. HW connection aktif.
4. High Impedance Digital.
5. Tidak memerlukan interrupt.
6. Input synchronization boleh synchronized/default karena hanya 57600 baud.

## E8. `HostTx`

1. Tambahkan satu Digital Output Pin.
2. Nama:

```text
HostTx
```

3. HW connection aktif.
4. Strong Drive.
5. Slew rate default/fast.

## E9. `StatusLED`

1. Tambahkan satu Digital Output Pin.
2. Nama:

```text
StatusLED
```

3. **HW connection tidak dicentang**, karena LED dikontrol API software.
4. Drive mode: Strong Drive.
5. Initial value: `1`.

LED biru CY8CKIT-059 pada P2[1] bersifat active-low:

```text
Write(0) = LED menyala
Write(1) = LED mati
```

**Checkpoint E:** TopDesign memiliki sembilan instance dengan nama persis:

```text
LoopIn
HystOut
LoopEdgeISR
SampleClock
SampleDMA
HostUART
HostRx
HostTx
StatusLED
```

---

# Bagian F — menggambar wire TopDesign

Pilih tool wire atau tekan shortcut wire yang ditampilkan Creator. Buat koneksi
berikut.

## F1. Feedback hysteresis

Hubungkan terminal output digital `LoopIn` ke terminal input `HystOut`:

```text
LoopIn ----------------------------> HystOut
```

Ini bukan software feedback. Fabric PSoC membuat P12[3] mengikuti P12[2] dengan
delay rendah.

## F2. Interrupt trigger

Hubungkan terminal interrupt `LoopIn` ke input `LoopEdgeISR`:

```text
LoopIn interrupt ------------------> LoopEdgeISR int_signal
```

Pastikan wire berasal dari terminal interrupt, bukan dari terminal data biasa.

## F3. DMA sample request

Hubungkan output `SampleClock` ke `drq` pada `SampleDMA`:

```text
SampleClock ------------------------> SampleDMA drq
```

## F4. UART

Hubungkan:

```text
HostRx -----------------------------> HostUART rx
HostUART tx ------------------------> HostTx
```

Tidak ada wire ke `StatusLED` karena pin itu software-controlled.

TopDesign final secara konseptual:

```text
                              +------> HystOut
                              |
LoopIn data ------------------+
LoopIn interrupt -------------------> LoopEdgeISR

SampleClock ------------------------> SampleDMA drq

HostRx -----------------------------> HostUART RX
HostUART TX ------------------------> HostTx

StatusLED        [software pin, tanpa wire]
```

**Checkpoint F:** tidak ada wire merah/putus dan semua terminal yang diperlukan
terhubung.

---

# Bagian G — pin assignment di `.cydwr`

1. Double-click `OneLoopDecoder.cydwr`.
2. Buka tab **Pins**.
3. Cari setiap signal/component pin.
4. Isi assignment persis:

| Signal | Assignment | Fungsi |
|---|---|---|
| `LoopIn` | `P12[2]` | output rangkaian input/phase conditioner |
| `HystOut` | `P12[3]` | feedback melalui R11 4.87 kOhm |
| `HostRx` | `P12[6]` | UART RX dari KitProg/host |
| `HostTx` | `P12[7]` | UART TX ke KitProg/host |
| `StatusLED` | `P2[1]` | LED biru onboard active-low |

Jangan assign P15[0] dan P15[1] sebagai GPIO. Kedua pin tersebut akan dipakai
MHz ECO/crystal.

Jika Creator memberi konflik P12[6]/P12[7], pastikan tidak ada USBFS atau
komponen pin lain yang menggunakan keduanya.

**Checkpoint G:** tidak ada dua signal pada pin yang sama dan device tetap
CY8C5888LTI-LP097.

---

# Bagian H — clock 5 MHz, PLL, CPU 80 MHz, sample 20 MHz

Buka tab **Clocks** pada `.cydwr`.

## H1. External crystal/ECO

1. Aktifkan MHz External Crystal Oscillator (`MHzECO`, `XTAL`, atau nama sejenis
   pada clock editor Creator).
2. Set nominal crystal frequency:

```text
5.000 MHz
```

3. Set startup agar ECO dimulai saat boot.
4. Jika diminta accuracy, isi nilai PPM crystal; jika tidak diketahui gunakan
   default sementara dan catat untuk diperiksa.

Hardware decoder menggunakan crystal 5 MHz di antara P15[0] dan P15[1] dengan
kapasitor 22 pF ke ground pada tiap sisi.

## H2. PLL

1. Set PLL input/source ke MHzECO/external crystal.
2. Set PLL multiplier/divider sehingga output menjadi:

```text
80 MHz
```

Untuk input 5 MHz, targetnya setara pengalian 16.

## H3. Master dan BUS clock

Atur:

```text
Master Clock = PLL output / 1 = 80 MHz
BUS_CLK      = Master / 1     = 80 MHz
```

## H4. Periksa SampleClock

Pada clock summary, `SampleClock` harus menunjukkan:

```text
20.000 MHz
```

Biasanya Creator memilih BUS_CLK / 4.

Konstanta firmware bergantung pada:

```text
80,000 CPU cycles per millisecond
20,000 CPU cycles per quarter millisecond
16 DMA samples per data bit
4 DMA samples per carrier cycle
```

Jangan melanjutkan jika CPU/BUS bukan 80 MHz atau SampleClock bukan 20 MHz.

**Checkpoint H:** Clock summary: ECO 5 MHz, PLL/Master/BUS 80 MHz,
SampleClock 20 MHz.

---

# Bagian I — memasukkan source firmware

## I1. Lokasi source

Source yang harus dipakai:

```text
firmware\psoc5lp-one-loop\OneLoopDecoder.cydsn\
```

File C:

```text
main.c
capture_dma.c
passage_tracker.c
rch_demod.c
rch_protocol.c
serial_console.c
timebase.c
```

File header:

```text
capture_dma.h
passage_tracker.h
rch_demod.h
rch_protocol.h
serial_console.h
timebase.h
```

Ada juga `cyapicallbacks.h` dan `PROJECT-MANIFEST.txt`. File manifest hanya
referensi dan tidak perlu dikompilasi.

## I2. Mengganti main.c

Project kosong biasanya sudah mempunyai `main.c`.

Cara aman:

1. Simpan project.
2. Tutup `main.c` di editor Creator.
3. Di Windows Explorer, backup `main.c` kosong jika diperlukan.
4. Salin `main.c` dari repository dan overwrite `main.c` milik project.
5. Kembali ke Creator dan pilih reload jika ditanya bahwa file berubah.

Jangan memiliki dua file `main.c` aktif di project.

## I3. Menambahkan file lain

1. Salin semua file `.c` dan `.h` di atas ke folder fisik
   `OneLoopDecoder.cydsn` milik project Windows.
2. Di Workspace Explorer, klik kanan project `OneLoopDecoder`.
3. Pilih **Add > Existing Item...**.
4. Pilih semua file C selain `main.c` yang sudah terdaftar.
5. Ulangi untuk header jika ingin semuanya terlihat di Workspace Explorer.
6. Jangan menyalin atau mengedit folder `Generated_Source` dari repository lain.

## I4. Generate Application

1. Simpan semua file dan TopDesign.
2. Pilih **Build > Generate Application**.
3. Buka `Generated_Source\PSoC5\project.h`.
4. Pastikan ada include/generated API untuk:

```text
LoopIn
LoopEdgeISR
SampleClock
SampleDMA
HostUART
StatusLED
```

Jika nama tersebut tidak ada, perbaiki nama instance TopDesign sebelum build.

---

# Bagian J — build firmware

## J1. Clean dan build

Jalankan berurutan:

1. **Build > Clean OneLoopDecoder**;
2. **Build > Generate Application**;
3. **Build > Build OneLoopDecoder**.

Lihat tab **Output** di bawah. Target pertama adalah:

```text
0 Errors
```

Warning dari fitter/clock juga harus dibaca; jangan hanya melihat bahwa HEX
berhasil dibuat.

## J2. Periksa hasil build

Cari file output, biasanya di salah satu folder:

```text
OneLoopDecoder.cydsn\CortexM3\ARM_GCC_541\Debug\OneLoopDecoder.hex
```

atau folder toolchain/configuration serupa.

Periksa report:

1. Device `CY8C5888LTI-LP097`.
2. BUS clock 80 MHz.
3. SampleClock 20 MHz.
4. Tidak ada pin placement error.
5. Tidak ada DMA routing error.
6. SRAM cukup untuk buffer DMA 4096 byte.
7. Capture buffer tidak menyeberangi batas 64 KiB. Firmware juga melakukan
   runtime check dan akan melaporkan DMA initialization failed jika salah.

## J3. Error yang umum

### `SampleDMA_DmaInitialize` undefined

Penyebab:

- komponen DMA tidak ada;
- instance bukan `SampleDMA`;
- application belum di-generate.

Perbaikan: rename komponen, Generate Application, Clean, Build.

### `LoopIn__PS` undefined

Penyebab:

- `LoopIn` bukan hardware digital input;
- HW connection tidak aktif;
- nama instance berbeda.

### `LoopIn_0_INTR` undefined

Rising-edge interrupt belum diaktifkan pada customizer `LoopIn`.

### `LoopEdgeISR_StartEx` undefined

Interrupt component tidak bernama `LoopEdgeISR`.

### `HostUART_PutString` undefined

UART component tidak bernama `HostUART` atau TX belum diaktifkan.

### `StatusLED_Write` undefined

Pin tidak bernama `StatusLED` atau salah tipe.

### DMA/drq routing error

Periksa bahwa `SampleClock` hanya tersambung ke `SampleDMA drq` dan DMA hardware
request dipilih Rising Edge.

### Clock tidak bisa 80 MHz

Periksa:

- device benar;
- ECO benar-benar 5 MHz;
- PLL source ECO;
- divider Master/BUS;
- P15[0]/P15[1] tidak digunakan sebagai GPIO.

**Checkpoint J:** build selesai 0 error dan report clock sudah diperiksa.
Jangan flashing jika checkpoint ini belum lulus.

---

# Bagian K — programming dengan KitProg

## K1. Sebelum program

1. Gunakan board cadangan jika tersedia.
2. Lepaskan coax/amplifier.
3. Lepaskan supply eksternal decoder.
4. Hubungkan sisi KitProg ke PC.
5. Pastikan tidak ada PSoC Programmer lain yang sedang memegang KitProg.
6. Tutup serial terminal untuk sementara.

## K2. Pilih target

1. Di Creator pilih **Debug > Select Debug Target...**.
2. Expand node KitProg.
3. Pilih target PSoC `CY8C5888LTI-LP097`.
4. Jika ada beberapa KitProg, cocokkan serial number/perangkat yang baru
   dipasang.

## K3. Program

1. Pilih **Debug > Program**.
2. Jangan pilih Bootloader Host.
3. Tunggu proses erase, program, dan verify selesai.
4. Pastikan Output menampilkan programming successful.
5. Tekan tombol reset target sekali atau power-cycle melalui USB.

Firmware aplikasi rusak biasanya tidak membuat board brick. SWD KitProg masih
dapat memprogram ulang selama hardware/programmer normal.

**Checkpoint K:** Creator menyatakan programming dan verify berhasil.

---

# Bagian L — tes UART sebelum memasang loop

## L1. Terminal

1. Buka Device Manager dan catat COM KitProg USB-UART.
2. Buka Tera Term/PuTTY.
3. Pilih COM tersebut.
4. Set:

```text
Baud       57600
Data bits  8
Parity     None
Stop bits  1
Flow       None
Local echo opsional
```

5. Tekan reset target.

Banner yang diharapkan:

```text
YURLAPS ONELOOP 0.1 READY 57600 8N1; TYPE HELP
```

Jika tidak muncul:

- pastikan COM adalah KitProg USB-UART;
- bukan target USB connector;
- pastikan P12[6]/P12[7] benar;
- pastikan HostUART 57600;
- coba tekan reset;
- program ulang.

## L2. Perintah pertama

Ketik satu per satu, tekan Enter:

```text
HELP
VERSION
MODE
SELFTEST
STATUS
```

Hasil SELFTEST yang benar:

```text
SELFTEST PASS id=4961721 vector=F916EFDA353B28290B0BCF3C
```

SELFTEST menguji codec di PSoC, bukan loop analog. Jika SELFTEST gagal, jangan
lanjutkan ke loop.

Status awal tanpa loop kira-kira:

```text
STATUS t_qms=... triggers=0 windows=0 dma_errors=0 preambles=0 rejects=0 packets=0 passages=0 accepted=0 suppressed=0 dropped=0
```

Yang paling penting:

```text
dma_errors=0
```

**Checkpoint L:** banner, HELP, SELFTEST PASS, dan dma_errors=0.

---

# Bagian M — memasang PCB decoder/loop

Matikan power sebelum memasang.

## M1. Sambungan penting

- Output conditioner decoder ke `P12[2]`.
- `P12[3]` ke node input hanya melalui `R11 = 4.87 kOhm`.
- Crystal 5 MHz tetap antara P15[0]/P15[1].
- UART KitProg tetap P12[6]/P12[7].
- Semua ground mengikuti PCB decoder yang sudah bekerja.
- Jangan sambungkan center coax langsung ke P12[2].

## M2. Test pertama

1. Gunakan loop pendek yang sebelumnya terbukti reliable, idealnya <= 2.5 m.
2. Gunakan Cano amplifier yang diketahui bekerja.
3. Gunakan satu transponder dengan ID diketahui.
4. Letakkan transponder dekat tengah loop, sekitar 5-10 cm dahulu.
5. Nyalakan decoder.
6. Di terminal ketik:

```text
MODE DIAG
```

7. Gerakkan transponder melintasi loop secara perlahan.
8. LED biru P2[1] harus menyala sekitar 100 ms ketika packet valid ditemukan.

Contoh diagnostic berhasil:

```text
SCAN samples=4096 preambles=... rejects=... valid_candidates=... unique=1
PKT raw=F916EFDA353B28290B0BCF3C id=4961721 status=0 t_qms=1234 q=100 sample=811 valid=1
4BB5B9000004D2
PASS id=4961721 t_qms=1234 hits=3 q=98 status=510 raw=F916...
```

Nilai persis sample, timestamp, jumlah candidate, hits, dan quality akan berbeda.

`q` adalah digital demodulation confidence, bukan RSSI analog dan bukan skala
signal strength MYLAPS.

---

# Bagian N — membaca STATUS tanpa oscilloscope

Urutan diagnosis harus seperti ini.

## N1. `dma_errors` bukan nol

DMA gagal dialokasikan, buffer placement salah, TD gagal dikonfigurasi, atau
re-arm gagal. Jangan menyimpulkan masalah loop sebelum error DMA diperbaiki.

## N2. `triggers=0`

P12[2] tidak menghasilkan rising edge digital.

Periksa:

1. Loop/amplifier mendapat power.
2. Coax dan BNC.
3. Rangkaian Q1-Q4/C1/C2.
4. Bias R9/R10.
5. P12[2] assignment.
6. P12[3]/R11.
7. Transponder dan baterainya.

DMM hanya dapat memeriksa supply dan DC bias; DMM tidak dapat membuktikan bentuk
carrier 5 MHz.

## N3. Trigger/windows naik, preambles tetap nol

Ada aktivitas digital tetapi tidak cocok dengan `F9 16` differential packet.

Periksa:

- ECO benar 5 MHz;
- BUS 80 MHz;
- SampleClock 20 MHz;
- DMA request Rising Edge;
- sinyal terlalu berisik atau clipping;
- transponder bukan format legacy yang didukung;
- posisi transponder terlalu jauh.

## N4. Preamble naik, reject banyak, packets nol

Sebagian header terlihat tetapi payload gagal strict re-encoding. Kemungkinan:

- noise;
- clipping;
- edge sampling tidak stabil;
- request DMA salah;
- carrier/reference berbeda;
- signal conditioner belum memberi level digital bersih.

Ulangi dengan loop pendek, transponder dekat, motor/ESC mati.

## N5. Packets naik tetapi passages tidak naik langsung

Passage baru ditutup setelah tidak ada packet lagi selama 8 ms. Tunggu sebentar
setelah memindahkan transponder. Same ID juga ditahan 300 ms dari awal passage
untuk mencegah double count.

## N6. Passage dan record keluar

Format Cano normal:

```text
4BB5B900001234
```

- `4BB5B9` = ID 4961721 dalam hexadecimal;
- `00001234` = timestamp quarter-millisecond hexadecimal;
- akhir line = CRLF.

Sebelum dipakai YurLaps, ketik:

```text
MODE CANO
```

Diagnostic mode menghasilkan banyak teks dan dapat memperlambat UART; jangan
pakai MODE DIAG pada balapan.

---

# Bagian O — UART ke ESP32-C3

P12[7] adalah TX PSoC dan P12[6] adalah RX PSoC.

```text
P12[7] PSoC TX -> level shifter/divider -> ESP32 RX
P12[6] PSoC RX <- level-compatible buffer <- ESP32 TX
GND PSoC ------------------------------ GND ESP32
```

PSoC dapat memakai I/O 5 V sedangkan ESP32-C3 tidak 5-V tolerant. Jangan
menghubungkan PSoC TX langsung ke ESP RX kecuali schematic/pengukuran sudah
membuktikan levelnya maksimum 3.3 V.

Untuk output satu arah, P12[6] tidak wajib jika ESP hanya menerima record.
Namun RX berguna untuk perintah STATUS/MODE.

---

# Bagian P — recovery dan rollback

## P1. Creator tidak menemukan target

1. Tutup terminal dan PSoC Programmer.
2. Lepaskan power/coax eksternal.
3. Cabut-pasang KitProg.
4. Coba port USB langsung, bukan hub pasif.
5. Buka PSoC Programmer dan periksa/update KitProg.
6. Kembali ke **Debug > Select Debug Target**.
7. Jika perlu tahan reset target saat acquisition dimulai, lalu lepas.

## P2. Firmware hidup tetapi UART mati

1. Pastikan memakai COM KitProg USB-UART.
2. Periksa 57600 8-N-1, no flow.
3. Periksa HostRx=P12[6], HostTx=P12[7].
4. Pastikan bukan target USB connector.
5. Clean, Generate, Build, Program ulang.

## P3. Mengembalikan HEX lama

1. Buka PSoC Programmer.
2. Pilih KitProg dan device CY8C5888LTI-LP097.
3. Pilih HEX lama yang sudah disimpan.
4. Program dan Verify.
5. Matikan power.
6. Kembalikan wiring sesuai foto.
7. Uji dengan software/terminal asli.

---

# Checklist akhir build pertama

Jangan lewatkan satu pun:

```text
[ ] Device CY8C5888LTI-LP097
[ ] ARM GCC
[ ] LoopIn bernama persis dan berada di P12[2]
[ ] HystOut bernama persis dan berada di P12[3]
[ ] LoopIn data terhubung ke HystOut
[ ] LoopIn interrupt terhubung ke LoopEdgeISR
[ ] SampleClock 20 MHz terhubung ke SampleDMA drq
[ ] SampleDMA request Rising Edge
[ ] HostUART 57600 8-N-1
[ ] HostRx P12[6]
[ ] HostTx P12[7]
[ ] StatusLED P2[1], initial 1, software-controlled
[ ] MHzECO 5 MHz pada P15[0]/P15[1]
[ ] PLL/Master/BUS 80 MHz
[ ] Generate Application berhasil
[ ] Build 0 errors
[ ] Program + Verify berhasil
[ ] Banner UART muncul
[ ] SELFTEST PASS
[ ] STATUS dma_errors=0
[ ] Test pertama memakai loop pendek dan transponder dekat
[ ] Firmware lama tersedia untuk rollback
```

Saat menemui tampilan atau error yang berbeda, jangan mengubah nama komponen
secara acak. Simpan screenshot **TopDesign**, tab **Pins**, tab **Clocks**, dan
seluruh teks error pada **Output** agar kesalahan dapat ditentukan dengan pasti.
