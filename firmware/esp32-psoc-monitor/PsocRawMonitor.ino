/*
 * ESP32-C3 monitor untuk menguji output OneLoopDecoder PSoC 5LP.
 *
 * PSoC P12[7] TX -> ESP32 GPIO2 RX
 * PSoC GND      -> ESP32 GND
 * UART PSoC: 57600 8N1, record-only, CRLF
 *
 * USB Serial ESP32 hanya untuk menampilkan hasil pemeriksaan. Jangan
 * sambungkan USB Serial ini ke input UART PSoC.
 */
#include <Arduino.h>

static constexpr int PSOC_RX_PIN = 2;
static constexpr int PSOC_TX_PIN = 5; // tidak dipakai; disediakan agar pin UART lengkap
static constexpr uint32_t PSOC_BAUD = 57600;
static constexpr size_t FRAME_LEN = 25;

HardwareSerial PsocSerial(1);
char frame[FRAME_LEN + 1];
size_t frameLength = 0;
uint32_t goodFrames = 0;
uint32_t badFrames = 0;

bool isHex(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}

uint32_t hexValue(const char *p, size_t count) {
  uint32_t value = 0;
  for (size_t i = 0; i < count; ++i) {
    char c = p[i];
    value <<= 4;
    if (c >= '0' && c <= '9') value |= uint32_t(c - '0');
    else if (c >= 'A' && c <= 'F') value |= uint32_t(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') value |= uint32_t(c - 'a' + 10);
  }
  return value;
}

void printHexBytes(const char *p, size_t count) {
  Serial.print("bytes:");
  for (size_t i = 0; i < count; ++i) {
    Serial.printf(" %02X", static_cast<unsigned char>(p[i]));
  }
  Serial.println();
}

void rejectFrame(const char *reason) {
  ++badFrames;
  Serial.print("BAD PSoC frame: ");
  Serial.println(reason);
  if (frameLength != 0) {
    Serial.print("raw: ");
    Serial.write(reinterpret_cast<const uint8_t *>(frame), frameLength);
    Serial.println();
    printHexBytes(frame, frameLength);
  }
  Serial.printf("good=%lu bad=%lu\n", (unsigned long)goodFrames,
                (unsigned long)badFrames);
}

void handleFrame() {
  frame[frameLength] = '\0';

  // OneLoopDecoder emits exactly:
  // nnnnnntttttttt-idhhqqvvtm\r\n
  if (frameLength != FRAME_LEN) {
    rejectFrame("panjang bukan 25 karakter");
    return;
  }
  if (frame[14] != '-') {
    rejectFrame("tanda '-' tidak berada pada posisi 15");
    return;
  }
  for (size_t i = 0; i < FRAME_LEN; ++i) {
    if (i == 14) continue;
    if (!isHex(frame[i])) {
      rejectFrame("ada karakter bukan hexadecimal");
      return;
    }
  }

  const uint32_t id = hexValue(frame + 0, 6);
  const uint32_t timeQms = hexValue(frame + 6, 8);
  const uint32_t decoderId = hexValue(frame + 15, 2);
  const uint32_t hits = hexValue(frame + 17, 2);
  const uint32_t quality = hexValue(frame + 19, 2);
  const uint32_t voltage = hexValue(frame + 21, 2);
  const uint32_t temperature = hexValue(frame + 23, 2);

  ++goodFrames;
  Serial.print("OK  raw: ");
  Serial.write(reinterpret_cast<const uint8_t *>(frame), FRAME_LEN);
  Serial.println();
  Serial.printf("    ID=%lu  time_qms=%lu (%.3f s)  decoder=%02lX\n",
                (unsigned long)id, (unsigned long)timeQms,
                double(timeQms) / 4000.0, (unsigned long)decoderId);
  Serial.printf("    hits=%lu  quality=%lu  voltage=%02lX  temperature=%02lX\n",
                (unsigned long)hits, (unsigned long)quality,
                (unsigned long)voltage, (unsigned long)temperature);
  Serial.printf("    total good=%lu bad=%lu\n",
                (unsigned long)goodFrames, (unsigned long)badFrames);
}

void readPsoc() {
  while (PsocSerial.available()) {
    const char c = static_cast<char>(PsocSerial.read());

    if (c == '\r') continue;
    if (c == '\n') {
      if (frameLength != 0) handleFrame();
      frameLength = 0;
      continue;
    }

    if (frameLength < FRAME_LEN) {
      frame[frameLength++] = c;
    } else {
      // Simpan sampai LF sebagai frame buruk, tetapi batasi buffer.
      frameLength = FRAME_LEN + 1;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("YurLaps PSoC OneLoopDecoder UART monitor");
  Serial.println("PSoC TX=P12[7] -> ESP32 GPIO2 RX, 57600 8N1");
  Serial.println("Menunggu record 25 karakter + CRLF...");

  // Tidak ada teks debug pada PsocSerial; hanya membaca TX PSoC.
  PsocSerial.begin(PSOC_BAUD, SERIAL_8N1, PSOC_RX_PIN, PSOC_TX_PIN);
}

void loop() {
  readPsoc();
}
