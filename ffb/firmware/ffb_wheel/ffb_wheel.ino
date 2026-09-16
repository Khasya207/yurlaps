// ============================================================================
//  YurFFB — firmware DIY Force Feedback wheel/joystick
//  Bagian dari proyek "ffb/" (lihat ffb/README.md).
//
//  Arsitektur:
//    - Aplikasi PC (FfbBridge) menghitung gaya FFB dari game (via vJoy),
//      lalu mengirim perintah torsi lewat serial ke sini.
//    - Firmware ini menjalankan loop kontrol 1 kHz:
//        * baca encoder (multi-turn) + estimasi kecepatan
//        * terapkan torsi host (dibatasi slew-rate & maxTorque)
//        * tambahkan damper lokal + endstop lunak (keselamatan)
//        * watchdog: host diam > WATCHDOG_MS -> torsi host dipaksa 0
//    - Telemetri (sudut/kecepatan/status) dikirim balik ke aplikasi
//      supaya game "melihat" sumbu setir bergerak.
//
//  Hardware default (lihat config.h): BTS7960 + AS5600 + Arduino Uno/Nano.
// ============================================================================

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "protocol.h"
#include "link.h"
#include "motor.h"
#include "encoder.h"

// ---------------------------------------------------------------------------
// Config runtime (bisa diubah dari aplikasi, bisa disimpan ke EEPROM)
// ---------------------------------------------------------------------------
struct FfbConfig {
  uint16_t maxTorque;      // mPct (1..1000)
  int16_t  slew;           // mPct per ms
  int16_t  softMin;        // derajat
  int16_t  softMax;        // derajat
  int16_t  endstopK;       // mPct per derajat
  int16_t  endstopD;       // mPct per (derajat/detik)
  int16_t  damper;         // mPct per (derajat/detik)
  uint8_t  flags;          // YURFFB_CFG_*
  uint8_t  reserved;
};
static_assert(sizeof(FfbConfig) == 16, "layout config harus tepat 16 byte");

static void configDefaults(FfbConfig &c) {
  c.maxTorque = 1000;
  c.slew      = 40;        // full swing 50 ms
  c.softMin   = -420;
  c.softMax   = 420;
  c.endstopK  = 80;        // 10 derajat melampaui endstop -> ~80% duty
  c.endstopD  = 20;
  c.damper    = 3;         // redaman fisik ringan, bikin setir terasa "padat"
  c.flags     = 0;
  c.reserved  = 0;
}

// ---------------------------------------------------------------------------
// State global
// ---------------------------------------------------------------------------
static SerialLink    link;
static MotorDriver   motor;
static AngleEncoder  enc;

static FfbConfig cfg;
static bool     enabled       = false;
static int16_t  hostTorque    = 0;     // permintaan torsi dari aplikasi (mPct)
static int16_t  appliedTorque = 0;     // setelah slew-limit
static int16_t  cmdTorque     = 0;     // perintah final ke motor (mPct)
static bool     encOk         = true;
static bool     watchdogTrip  = false;
static uint8_t  stateFlags    = 0;
static uint16_t telemPeriodMs = TELEM_PERIOD_MS;
static uint32_t lastTelemMs   = 0;
static uint32_t seq           = 0;
static uint16_t lastLoopUs    = 0;
static uint32_t lastCtrlUs    = 0;

// ---------------------------------------------------------------------------
// EEPROM (layout: magic u32 | ver u8 | config 16B | crc8) = 22 byte
// ---------------------------------------------------------------------------
#define EE_MAGIC 0x59465742UL   // "YFWB"
#define EE_VER   1

static void eeWrite8(uint16_t a, uint8_t v) { EEPROM.write(a, v); }
static uint8_t eeRead8(uint16_t a) { return EEPROM.read(a); }

static void saveConfigEeprom() {
  uint8_t blob[16];
  memcpy(blob, &cfg, 16);
  eeWrite8(0, EE_MAGIC & 0xFF);
  eeWrite8(1, (EE_MAGIC >> 8) & 0xFF);
  eeWrite8(2, (EE_MAGIC >> 16) & 0xFF);
  eeWrite8(3, (EE_MAGIC >> 24) & 0xFF);
  eeWrite8(4, EE_VER);
  for (uint8_t i = 0; i < 16; i++) eeWrite8(5 + i, blob[i]);
  uint8_t crc = SerialLink::crc8(&blob[0], 16) ^ EE_VER;  // ikutkan ver
  eeWrite8(21, crc);
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.commit();
#endif
}

static bool loadConfigEeprom() {
  uint32_t magic = (uint32_t)eeRead8(0) | ((uint32_t)eeRead8(1) << 8) |
                   ((uint32_t)eeRead8(2) << 16) | ((uint32_t)eeRead8(3) << 24);
  if (magic != EE_MAGIC) return false;
  uint8_t ver = eeRead8(4);
  if (ver != EE_VER) return false;
  uint8_t blob[16];
  for (uint8_t i = 0; i < 16; i++) blob[i] = eeRead8(5 + i);
  uint8_t crc = SerialLink::crc8(&blob[0], 16) ^ EE_VER;
  if (crc != eeRead8(21)) return false;
  memcpy(&cfg, blob, 16);
  return true;
}

// ---------------------------------------------------------------------------
// Helper little-endian untuk payload
// ---------------------------------------------------------------------------
static void putU16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
static void putI16(uint8_t *p, int16_t v)  { putU16(p, (uint16_t)v); }
static void putU32(uint8_t *p, uint32_t v) {
  p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}
static void putI32(uint8_t *p, int32_t v)  { putU32(p, (uint32_t)v); }
static uint16_t getU16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  getI16(const uint8_t *p) { return (int16_t)getU16(p); }

// ---------------------------------------------------------------------------
// Pesan ke host
// ---------------------------------------------------------------------------
static void sendPong() {
  uint8_t p[11] = { 'Y','U','R','F','F','B',
                    YURFFB_PROTO_VER, YURFFB_FW_MAJOR, YURFFB_FW_MINOR,
                    0, 0 };
  p[9]  = enc.typeCode();
  p[10] = DRIVER_TYPE;
  link.sendFrame(YURFFB_MSG_PONG, p, 11);
}

static void sendState() {
  uint8_t p[15];
  putU32(&p[0], seq);
  putI32(&p[4], enc.angleMdeg());
  putI16(&p[8], (int16_t)enc.velocityDdegS());
  putI16(&p[10], cmdTorque);
  p[12] = stateFlags;
  putU16(&p[13], lastLoopUs);
  link.sendFrame(YURFFB_MSG_STATE, p, 15);
}

static void sendConfigEcho() {
  uint8_t p[16];
  memcpy(p, &cfg, 16);
  link.sendFrame(YURFFB_MSG_CONFIG, p, 16);
}

// ---------------------------------------------------------------------------
// Dispatch perintah dari host
// ---------------------------------------------------------------------------
static void applyConfigFlags() {
  enc.setInvert(cfg.flags & YURFFB_CFG_INV_ENCODER);
}

static void handleFrame(uint8_t cmd, const uint8_t *payload, uint8_t len) {
  switch (cmd) {
    case YURFFB_CMD_PING:
      sendPong();
      break;

    case YURFFB_CMD_ENABLE: {
      bool on = (len >= 1) && (payload[0] != 0);
      if (on && !encOk) {
        link.sendLog("ENABLE ditolak: encoder fault");
        motor.setEnable(false);
        enabled = false;
        break;
      }
      enabled = on;
      motor.setEnable(on);
      if (on) { appliedTorque = 0; hostTorque = 0; }
      break;
    }

    case YURFFB_CMD_TORQUE:
      if (len >= 2) hostTorque = getI16(payload);
      break;

    case YURFFB_CMD_SET_CONFIG: {
      if (len != YURFFB_CONFIG_PAYLOAD_LEN) { link.sendLog("config len salah"); break; }
      FfbConfig c;
      memcpy(&c, payload, 16);
      // sanitasi
      if (c.maxTorque < 1)   c.maxTorque = 1;
      if (c.maxTorque > ABS_MAX_TORQUE) c.maxTorque = ABS_MAX_TORQUE;
      if (c.slew < 0) c.slew = 0;
      if (c.softMin >= c.softMax) { c.softMin = -420; c.softMax = 420; }
      cfg = c;
      applyConfigFlags();
      sendConfigEcho();
      break;
    }

    case YURFFB_CMD_GET_STATE:
      sendState();
      break;

    case YURFFB_CMD_SET_CENTER:
      enc.setCenterAtCurrent();
      break;

    case YURFFB_CMD_SET_TELEM:
      if (len >= 2) telemPeriodMs = getU16(payload);
      break;

    case YURFFB_CMD_SAVE_CONFIG:
      saveConfigEeprom();
      link.sendLog("config disimpan");
      sendConfigEcho();
      break;

    case YURFFB_CMD_RESET_CFG:
      configDefaults(cfg);
      applyConfigFlags();
      saveConfigEeprom();
      link.sendLog("config reset");
      sendConfigEcho();
      break;

    default:
      break;   // perintah tak dikenal: abaikan (tetap reset watchdog — frame valid)
  }
}

// ---------------------------------------------------------------------------
// Loop kontrol — dipanggil tiap CTRL_PERIOD_US
// ---------------------------------------------------------------------------
static inline int32_t clampI32(int32_t v, int32_t lo, int32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void controlTick(uint32_t dtUs) {
  stateFlags = 0;

  encOk = enc.update();
  if (!encOk) stateFlags |= YURFFB_FL_ENC_FAULT;

  int32_t angle = enc.angleMdeg();
  int32_t velD  = enc.velocityDdegS();     // ddeg/s

  // ---- watchdog host ----
  uint32_t now = millis();
  watchdogTrip = (now - link.lastRxMs()) > WATCHDOG_MS;
  if (watchdogTrip) {
    stateFlags |= YURFFB_FL_WATCHDOG;
    hostTorque = 0;   // endstop & damper lokal tetap hidup sebagai jaring pengaman
  }

  if (!enabled) stateFlags |= YURFFB_FL_DISABLED;

  // ---- torsi host dengan slew-limit ----
  int32_t dtMs = (int32_t)(dtUs / 1000UL);
  if (dtMs < 1) dtMs = 1;
  int32_t maxStep = (int32_t)cfg.slew * dtMs;
  int32_t target = (enabled && encOk) ? clampI32(hostTorque, -(int32_t)cfg.maxTorque,
                                                       +(int32_t)cfg.maxTorque)
                                      : 0;
  int32_t step = target - appliedTorque;
  if (step >  maxStep) step =  maxStep;
  if (step < -maxStep) step = -maxStep;
  appliedTorque += step;

  int32_t out = appliedTorque;

  // ---- damper lokal (menghaluskan + keselamatan saat link lambat) ----
  if (encOk && cfg.damper) {
    int32_t damp = -(int32_t)cfg.damper * (velD / 10);   // mPct per deg/s
    out += damp;
  }

  // ---- endstop lunak ----
  if (encOk && cfg.softMin < cfg.softMax) {
    int32_t maxM = (int32_t)cfg.softMax * 1000;
    int32_t minM = (int32_t)cfg.softMin * 1000;
    if (angle > maxM) {
      int32_t over = clampI32(angle - maxM, 0, 30000);   // max 30 derajat dihitung
      int32_t f = ((int32_t)cfg.endstopK * over) / 1000;
      if (velD > 0) f -= (int32_t)cfg.endstopD * (velD / 10);
      out += clampI32(f, 0, ABS_MAX_TORQUE);
      stateFlags |= YURFFB_FL_ENDSTOP;
    } else if (angle < minM) {
      int32_t over = clampI32(minM - angle, 0, 30000);
      int32_t f = -(((int32_t)cfg.endstopK * over) / 1000);
      if (velD < 0) f += (int32_t)cfg.endstopD * (-velD / 10);
      out += clampI32(f, -ABS_MAX_TORQUE, 0);
      stateFlags |= YURFFB_FL_ENDSTOP;
    }
  }

  // ---- clamp final ----
  int32_t lim = (int32_t)cfg.maxTorque;
  if (lim > ABS_MAX_TORQUE) lim = ABS_MAX_TORQUE;
  int32_t cmdT = clampI32(out, -lim, lim);
  if (cmdT != out) stateFlags |= YURFFB_FL_CLAMPED;

  // ---- invert motor (pemasangan kabel terbalik) ----
  if (cfg.flags & YURFFB_CFG_INV_MOTOR) cmdT = -cmdT;
  cmdTorque = (int16_t)cmdT;
  motor.setTorque(cmdTorque);
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------
void setup() {
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.begin(64);
#endif
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  link.begin(handleFrame);
  motor.begin();

  if (!loadConfigEeprom()) {
    configDefaults(cfg);
    link.sendLog("config default (EEPROM kosong/korup)");
  }
  applyConfigFlags();

  encOk = enc.begin();
  if (!encOk) link.sendLog("ENCODER GAGAL — cek wiring!");
  else        link.sendLog("encoder ok");

#if DEMO_SELF_TEST
  enabled = true;
  motor.setEnable(true);
#endif
}

void loop() {
  link.poll();   // baca serial secepatnya (non-blocking)

  uint32_t now = micros();
  if (now - lastCtrlUs >= CTRL_PERIOD_US) {
    uint32_t dt = now - lastCtrlUs;
    lastCtrlUs = now;

#if DEMO_SELF_TEST
    // mode tes tanpa aplikasi: pegas center sederhana
    int32_t a = enc.angleMdeg();
    int32_t v = enc.velocityDdegS();
    hostTorque = (int16_t)clampI32(-(a / 25) - (v / 12), -600, 600);
    watchdogTrip = false;
#endif

    controlTick(dt);
    lastLoopUs = (uint16_t)(micros() - now);
  }

  // telemetri periodik
  uint32_t ms = millis();
  if (telemPeriodMs && (ms - lastTelemMs) >= telemPeriodMs) {
    lastTelemMs = ms;
    seq++;
    sendState();
  }

  // LED status: solid = aktif, blink cepat = fault, blink lambat = standby
#if !DEMO_SELF_TEST
  bool led;
  if (!encOk)          led = (ms / 120) % 2;
  else if (enabled)    led = true;
  else                 led = (ms / 600) % 2;
  digitalWrite(PIN_STATUS_LED, led ? HIGH : LOW);
#else
  digitalWrite(PIN_STATUS_LED, (ms / 120) % 2 ? HIGH : LOW);
#endif
}
