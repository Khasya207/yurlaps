// ============================================================================
//  Unit-test firmware YurFFB di host PC (mock Arduino).
//  Sertakan firmware via .ino + modul .cpp asli, lalu simulasi:
//    - framing serial + CRC
//    - perintah PING/ENABLE/TORQUE/SET_CONFIG/SET_CENTER
//    - slew-limit, watchdog, endstop lunak, invert motor, encoder fault
//    - persistensi EEPROM
//  Jalankan: make -C ffb/firmware/test
// ============================================================================
#include "Arduino.h"
#include "Wire.h"
#include "EEPROM.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// Implementasi mock
// ---------------------------------------------------------------------------
unsigned long g_mockMillis = 0, g_mockMicros = 0;
uint8_t  TCCR1A = 0, TCCR1B = 0;
uint16_t ICR1 = 0, OCR1A = 0, OCR1B = 0;
uint8_t  g_mockPinModes[32] = {0};
uint8_t  g_mockPinState[32] = {0};
int      g_mockAnalogValue[8] = {512};
void (*g_mockIsr[3])() = {nullptr, nullptr, nullptr};
MockSerial Serial;
MockWire   Wire;
MockEeprom EEPROM;

unsigned long millis() { return g_mockMillis; }
unsigned long micros() { return g_mockMicros; }
void delay(unsigned long ms) { g_mockMillis += ms; }
void pinMode(uint8_t pin, uint8_t mode) { if (pin < 32) g_mockPinModes[pin] = mode; }
void digitalWrite(uint8_t pin, uint8_t val) { if (pin < 32) g_mockPinState[pin] = val; }
int digitalRead(uint8_t pin) { return (pin < 32) ? g_mockPinState[pin] : 0; }
void analogWrite(uint8_t pin, int val) { if (pin < 32) g_mockPinState[pin] = (uint8_t)val; }
int analogRead(uint8_t pin) { return g_mockAnalogValue[(pin - 14) & 7]; }
void attachInterrupt(uint8_t irq, void (*isr)(), int mode) {
  (void)mode;
  if (irq < 3) g_mockIsr[irq] = isr;
}
int digitalPinToInterrupt(uint8_t pin) { return (pin >= 2 && pin <= 3) ? (pin - 2) : -1; }

int MockSerial::read() {
  if (rxBuf.empty()) return -1;
  int v = rxBuf.front();
  rxBuf.pop_front();
  return v;
}
size_t MockSerial::write(uint8_t b) { txBytes.push_back(b); return 1; }
size_t MockSerial::write(const uint8_t *buf, size_t n) {
  for (size_t i = 0; i < n; i++) txBytes.push_back(buf[i]);
  return n;
}

// ---------------------------------------------------------------------------
// Firmware under test
// ---------------------------------------------------------------------------
#include "../ffb_wheel/ffb_wheel.ino"

// ---------------------------------------------------------------------------
// Helper test
// ---------------------------------------------------------------------------
static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { \
  if (cond) { g_pass++; } \
  else { g_fail++; printf("FAIL line %d: %s\n", __LINE__, #cond); } \
} while (0)

struct TFrame { uint8_t cmd; std::vector<uint8_t> p; };

static void sendCmd(uint8_t cmd, const uint8_t *payload, uint8_t n) {
  uint8_t body[40];
  body[0] = cmd;
  if (n) memcpy(&body[1], payload, n);
  uint8_t crc = SerialLink::crc8(body, n + 1);
  uint8_t f[45];
  f[0] = 0xAA; f[1] = 0x55; f[2] = (uint8_t)(n + 2);   // LEN = cmd + payload + crc
  memcpy(&f[3], body, n + 1);
  f[3 + n + 1] = crc;
  Serial.feed(f, (size_t)(n + 5));   // SOF2 + LEN + (cmd+payload+crc)
}

static std::vector<TFrame> parseTx() {
  std::vector<TFrame> out;
  auto &v = Serial.txBytes;
  size_t i = 0;
  while (i + 3 <= v.size()) {
    if (v[i] != 0xAA || v[i + 1] != 0x55) { i++; continue; }
    uint8_t len = v[i + 2];
    if (len < 2 || len > 40 || i + 3 + len > v.size()) { i++; continue; }
    uint8_t crc = SerialLink::crc8(&v[i + 3], len - 1);
    if (crc == v[i + 3 + len - 1]) {
      TFrame fr;
      fr.cmd = v[i + 3];
      for (uint8_t k = 0; k < len - 2; k++) fr.p.push_back(v[i + 4 + k]);
      out.push_back(fr);
      i += 3 + (size_t)len;
    } else {
      i++;
    }
  }
  return out;
}

static bool hasMsg(const std::vector<TFrame> &fr, uint8_t cmd, TFrame *out = nullptr) {
  for (auto &f : fr) if (f.cmd == cmd) { if (out) *out = f; return true; }
  return false;
}

// jalankan n siklus loop (1 ms per siklus), sambil majukan waktu mock
static void runTicks(int n) {
  for (int i = 0; i < n; i++) {
    g_mockMicros += 1000;
    g_mockMillis += 1;
    loop();
  }
}

static int16_t lastState(TFrame *st) {
  std::vector<TFrame> fr = parseTx();
  TFrame last{};
  bool found = false;
  for (auto &f : fr) if (f.cmd == YURFFB_MSG_STATE) { last = f; found = true; }
  if (!found) return -32768;
  if (st) *st = last;
  if (last.p.size() != 15) return -32767;
  return (int16_t)(last.p[10] | (last.p[11] << 8));
}

int main() {
#if defined(TEST_SYNTAX_ONLY)
  return 0;
#endif
  printf("== YurFFB firmware host-test (encoder type %d) ==\n", ENCODER_TYPE);

  // ---- 1. CRC-8 vektor standar ("123456789" -> 0xF4, CRC-8/SMBUS) ----
  CHECK(SerialLink::crc8((const uint8_t *)"123456789", 9) == 0xF4);

  // ---- 2. boot + PING -> PONG ----
  setup();
  Serial.clearTx();
  sendCmd(YURFFB_CMD_PING, nullptr, 0);
  runTicks(1);
  TFrame pong;
  CHECK(hasMsg(parseTx(), YURFFB_MSG_PONG, &pong));
  CHECK(pong.p.size() == 11 && memcmp(&pong.p[0], "YURFFB", 6) == 0);
  CHECK(pong.p.size() == 11 && pong.p[6] == YURFFB_PROTO_VER);

  // ---- 3. ENABLE ditolak saat encoder fault, diterima saat sehat ----
#if ENCODER_TYPE == ENCODER_AS5600
  Wire.nack = true;
  for (int i = 0; i < 20; i++) runTicks(1);      // provokasi fault
  uint8_t one = 1;
  sendCmd(YURFFB_CMD_ENABLE, &one, 1);
  runTicks(1);
  CHECK(g_mockPinState[PIN_MOTOR_EN] == LOW);    // ditolak
  Wire.nack = false;
  runTicks(15);                                   // fault hilang (recovery bertahap)
#endif
  // ENABLE sehat — berlaku untuk semua varian encoder
  {
    uint8_t one = 1;
    sendCmd(YURFFB_CMD_ENABLE, &one, 1);
    runTicks(1);
    CHECK(g_mockPinState[PIN_MOTOR_EN] == HIGH);
  }

  // ---- 4. TORQUE positif -> duty di channel A saja (BTS7960) ----
  Serial.clearTx();
  int16_t t500 = 500;
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&t500, 2);
  runTicks(1);
  // slew default 40 mPct/ms -> tick pertama sekitar 40 mPct
  CHECK(OCR1A > 0 && OCR1A <= 40);                // ~40*799/1000 = 31
  CHECK(OCR1B == 0);
  runTicks(20);                                   // menuju target
  CHECK(OCR1A > 300);                             // ~500*799/1000 = 399
  CHECK(OCR1B == 0);

  // ---- 5. TORQUE negatif -> duty pindah ke channel B ----
  Serial.clearTx();
  int16_t tm500 = -500;
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&tm500, 2);
  runTicks(30);
  CHECK(OCR1B > 300 && OCR1A == 0);

  // ---- 6. torsi dibatasi maxTorque (SET_CONFIG) + echo ----
  Serial.clearTx();
  uint8_t cfg16[16];
  {
    uint16_t maxT = 300; int16_t slew = 100, smin = -420, smax = 420,
             ek = 80, ed = 20, dp = 3; uint8_t fl = 0, rs = 0;
    memcpy(&cfg16[0], &maxT, 2); memcpy(&cfg16[2], &slew, 2);
    memcpy(&cfg16[4], &smin, 2); memcpy(&cfg16[6], &smax, 2);
    memcpy(&cfg16[8], &ek, 2);   memcpy(&cfg16[10], &ed, 2);
    memcpy(&cfg16[12], &dp, 2);  cfg16[14] = fl; cfg16[15] = rs;
  }
  sendCmd(YURFFB_CMD_SET_CONFIG, cfg16, 16);
  runTicks(1);
  TFrame echo;
  CHECK(hasMsg(parseTx(), YURFFB_MSG_CONFIG, &echo));
  CHECK(echo.p.size() == 16 && memcmp(&echo.p[0], cfg16, 16) == 0);
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&t500, 2);   // minta 500, dibatasi 300
  runTicks(30);
  CHECK(OCR1A > 200 && OCR1A <= 241);             // ~300*799/1000 = 239

  // ---- 7. watchdog: host diam -> torsi host 0 ----
  Serial.clearTx();
  runTicks(300);                                   // 300 ms tanpa frame
  TFrame st;
  CHECK(lastState(&st) == 0);
  CHECK((st.p[12] & YURFFB_FL_WATCHDOG) != 0);
  // frame baru memulihkan watchdog
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&t500, 2);
  runTicks(30);
  CHECK(lastState(nullptr) != 0);

  // ---- 8. endstop lunak ----
#if ENCODER_TYPE == ENCODER_AS5600
  Serial.clearTx();
  {
    uint16_t maxT = 1000; int16_t slew = 200, smin = -10, smax = 10,
             ek = 80, ed = 5, dp = 0; uint8_t fl = 0, rs = 0;
    memcpy(&cfg16[0], &maxT, 2); memcpy(&cfg16[2], &slew, 2);
    memcpy(&cfg16[4], &smin, 2); memcpy(&cfg16[6], &smax, 2);
    memcpy(&cfg16[8], &ek, 2);   memcpy(&cfg16[10], &ed, 2);
    memcpy(&cfg16[12], &dp, 2);  cfg16[14] = fl; cfg16[15] = rs;
    sendCmd(YURFFB_CMD_SET_CONFIG, cfg16, 16);
  }
  int16_t t0 = 0;
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&t0, 2);
  runTicks(5);
  // putar "roda" ke +15 derajat (AS5600: 4096 counts = 360 derajat)
  Wire.g_rawAngle = (uint16_t)((15L * 4096L) / 360L);   // = 170 counts
  runTicks(30);                                    // biarkan velocity mereda
  TFrame stEnd;
  int16_t tEnd = lastState(&stEnd);
  CHECK(tEnd > 0);                                 // pegas endstop mendorong balik
  CHECK((stEnd.p[12] & YURFFB_FL_ENDSTOP) != 0);
  // sudut terbaca ~15 derajat
  int32_t ang = (int32_t)stEnd.p[4] | ((int32_t)stEnd.p[5] << 8) |
                ((int32_t)stEnd.p[6] << 16) | ((int32_t)stEnd.p[7] << 24);
  CHECK(ang > 14000 && ang < 15500);               // ~15000 mdeg
#endif

  // ---- 9. SET_CENTER ----
#if ENCODER_TYPE == ENCODER_AS5600
  sendCmd(YURFFB_CMD_SET_CENTER, nullptr, 0);
  runTicks(3);
  TFrame stC;
  lastState(&stC);
  int32_t angC = (int32_t)stC.p[4] | ((int32_t)stC.p[5] << 8) |
                 ((int32_t)stC.p[6] << 16) | ((int32_t)stC.p[7] << 24);
  CHECK(angC >= -200 && angC <= 200);              // ~0 setelah re-center
#endif

#if ENCODER_TYPE == ENCODER_QUAD
  // ---- simulasi quadrature: 4 transisi CW = +4 counts ----
  const uint8_t seqA[5] = {0, 0, 1, 1, 0};
  const uint8_t seqB[5] = {0, 1, 1, 0, 0};
  for (int s = 0; s < 4; s++) {
    g_mockPinState[PIN_QUAD_A] = seqA[s + 1];
    g_mockPinState[PIN_QUAD_B] = seqB[s + 1];
    if (g_mockIsr[0]) g_mockIsr[0]();
    if (g_mockIsr[1]) g_mockIsr[1]();
  }
  runTicks(3);
  TFrame stQ;
  lastState(&stQ);
  int32_t angQ = (int32_t)stQ.p[4] | ((int32_t)stQ.p[5] << 8) |
                 ((int32_t)stQ.p[6] << 16) | ((int32_t)stQ.p[7] << 24);
  CHECK(angQ == 600);                              // 4 counts * 360000/2400
#endif

#if ENCODER_TYPE == ENCODER_POT
  // ---- potensiometer: raw 700 -> ~58.8 derajat dari center 511 ----
  g_mockAnalogValue[0] = 700;
  runTicks(3);
  TFrame stP;
  lastState(&stP);
  int32_t angP = (int32_t)stP.p[4] | ((int32_t)stP.p[5] << 8) |
                 ((int32_t)stP.p[6] << 16) | ((int32_t)stP.p[7] << 24);
  CHECK(angP > 57000 && angP < 60000);
#endif

  // ---- 10. invert motor ----
  {
    uint16_t maxT = 1000; int16_t slew = 200, smin = -420, smax = 420,
             ek = 80, ed = 20, dp = 3; uint8_t fl = 0x01, rs = 0;
    memcpy(&cfg16[0], &maxT, 2); memcpy(&cfg16[2], &slew, 2);
    memcpy(&cfg16[4], &smin, 2); memcpy(&cfg16[6], &smax, 2);
    memcpy(&cfg16[8], &ek, 2);   memcpy(&cfg16[10], &ed, 2);
    memcpy(&cfg16[12], &dp, 2);  cfg16[14] = fl; cfg16[15] = rs;
    sendCmd(YURFFB_CMD_SET_CONFIG, cfg16, 16);
  }
  sendCmd(YURFFB_CMD_TORQUE, (uint8_t *)&t500, 2);
  runTicks(30);
#if ENCODER_TYPE == ENCODER_AS5600 && DRIVER_TYPE == DRIVER_BTS7960
  CHECK(OCR1B > 300 && OCR1A == 0);                // polaritas terbalik
#endif

  // ---- 11. SAVE + reload EEPROM ----
  {
    uint16_t maxT = 777; int16_t slew = 40, smin = -420, smax = 420,
             ek = 80, ed = 20, dp = 3; uint8_t fl = 0, rs = 0;
    memcpy(&cfg16[0], &maxT, 2); memcpy(&cfg16[2], &slew, 2);
    memcpy(&cfg16[4], &smin, 2); memcpy(&cfg16[6], &smax, 2);
    memcpy(&cfg16[8], &ek, 2);   memcpy(&cfg16[10], &ed, 2);
    memcpy(&cfg16[12], &dp, 2);  cfg16[14] = fl; cfg16[15] = rs;
    sendCmd(YURFFB_CMD_SET_CONFIG, cfg16, 16);
  }
  sendCmd(YURFFB_CMD_SAVE_CONFIG, nullptr, 0);
  runTicks(2);
  uint8_t dummy = 1;
  sendCmd(YURFFB_CMD_ENABLE, &dummy, 1);   // jaga state enable utk cek LED/EN
  runTicks(2);
  setup();                                  // "reboot": harus memuat 777
  CHECK(cfg.maxTorque == 777);

  // ---- 12. resync parser setelah sampah ----
  Serial.clearTx();
  uint8_t junk[7] = {0x00, 0xAA, 0x37, 0xFF, 0x55, 0x01, 0x99};
  Serial.feed(junk, 7);
  sendCmd(YURFFB_CMD_PING, nullptr, 0);
  runTicks(1);
  CHECK(hasMsg(parseTx(), YURFFB_MSG_PONG));

  printf("== hasil: %d lulus, %d gagal ==\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
