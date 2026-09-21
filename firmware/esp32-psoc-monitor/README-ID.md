# Monitor ESP32 untuk OneLoopDecoder PSoC

Sketch ini membaca apa yang benar-benar dikirim firmware PSoC OneLoopDecoder melalui UART, memeriksa framing, lalu menampilkan hasilnya pada USB Serial ESP32.

## Sambungan

```text
PSoC P12[7] TX  -> ESP32-C3 GPIO2 (RX)
PSoC GND        -> ESP32 GND
```

Jangan menghubungkan PSoC TX ke GPIO5. GPIO5 hanya didefinisikan sebagai pin TX yang tidak dipakai pada sketch ini. Level UART harus kompatibel 3,3 V. Jangan memasukkan 5 V ke GPIO ESP32.

PSoC harus memakai:

```text
57600 baud
8 data bit
no parity
1 stop bit
```

Buka USB Serial ESP32 pada **115200 baud**. USB Serial hanya untuk melihat hasil pemeriksaan.

## Format yang diperiksa

OneLoopDecoder mengirim satu baris setelah passage selesai:

```text
nnnnnntttttttt-idhhqqvvtm\r\n
```

Panjang data sebelum CRLF adalah 25 karakter:

| Posisi | Panjang | Isi |
|---:|---:|---|
| 0–5 | 6 | ID transponder hexadecimal |
| 6–13 | 8 | waktu packet pertama, satuan quarter-millisecond |
| 14 | 1 | `-` |
| 15–16 | 2 | ID decoder, biasanya `01` |
| 17–18 | 2 | jumlah hit valid |
| 19–20 | 2 | quality digital `00`–`64` hexadecimal, bukan RSSI |
| 21–22 | 2 | tegangan, firmware ini selalu `00` |
| 23–24 | 2 | temperatur, firmware ini selalu `00` |

Contoh:

```text
23E35500075A06-01152A0000\r\n
```

Artinya secara umum:

```text
ID        = 0x23E355
waktu     = 0x00075A06 quarter-ms
decoder   = 0x01
hits      = 0x15 = 21
quality   = 0x2A = 42
voltage   = 0x00 (tidak tersedia)
temperature= 0x00 (tidak tersedia)
```

Sketch menolak baris yang panjangnya bukan 25 karakter, tanda minusnya salah posisi, atau mengandung karakter non-hexadecimal. Baris rusak ditampilkan dalam bentuk raw dan hexadecimal byte.

## Cara uji paling sederhana

1. Flash `PsocRawMonitor.ino` ke ESP32-C3.
2. Sambungkan GND PSoC dan GND ESP32.
3. Sambungkan P12[7] PSoC ke GPIO2 ESP32.
4. Nyalakan PSoC dengan OneLoopDecoder.
5. Buka Serial Monitor USB pada 115200.
6. Lewatkan transponder atau gunakan setup receiver yang sudah diketahui bekerja.
7. Pastikan keluar baris `OK raw:`.

Jika tidak ada data:

- periksa PSoC benar-benar memakai pin P12[7], bukan P12[6];
- periksa kabel ground bersama;
- pastikan UART PSoC 57600 8N1;
- pastikan ESP32 yang dipakai memang ESP32-C3 dan GPIO2 adalah RX yang tersambung;
- jangan memakai USB CDC PSoC sebagai pengganti UART TX P12[7].

Sketch ini sengaja tidak mengubah record, tidak mengirim perintah ke PSoC, dan tidak memakai Wi-Fi. Setelah format terbukti benar, parser ini dapat dipindahkan ke firmware YurLaps utama.
