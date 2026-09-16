#include "encoder.h"

#if ENCODER_TYPE == ENCODER_AS5600
#include <Wire.h>
#endif

// ============================ AS5600 (I2C) =================================
#if ENCODER_TYPE == ENCODER_AS5600

bool AngleEncoder::begin() {
#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
#else
  Wire.begin();
#endif
  Wire.setClock(I2C_CLOCK_HZ);
  uint16_t raw;
  return readRaw(raw);   // cek device menjawab
}

bool AngleEncoder::readRaw(uint16_t &raw) {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(AS5600_REG_ANGLE_H);
  if (Wire.endTransmission(false) != 0) return false;   // NACK / bus mati
  if (Wire.requestFrom((uint8_t)AS5600_ADDR, (uint8_t)2) != 2) return false;
  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  raw = ((uint16_t)(hi & 0x0F) << 8) | lo;              // 12-bit
  return true;
}

bool AngleEncoder::update() {
  uint16_t raw;
  if (!readRaw(raw)) {
    // fault: bertahap, biarkan sudut terakhir (jangan loncat)
    if (_i2cFaults < 255) _i2cFaults++;
    return _i2cFaults < 10;   // toleransi beberapa tick gagal
  }
  _i2cFaults = 0;

  if (_lastRawValid) {
    int32_t delta = (int32_t)raw - (int32_t)_lastRaw;
    // unwrap 12-bit (0..4095): lompatan > 2048 = melewati 0/4095
    if (delta >  2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    _totalCounts += delta;
  }
  _lastRaw = raw;
  _lastRawValid = true;

  int32_t rel = (int32_t)(( _totalCounts - (int64_t)_centerCounts));
  int64_t mdeg = (rel * 360000LL) / 4096LL;
  if (_invert) mdeg = -mdeg;

  uint32_t now = micros();
  if (_lastUpdateUs) {
    uint32_t dt = now - _lastUpdateUs;
    if (dt >= 500 && dt < 100000UL) {   // abaikan dt tak masuk akal
      int32_t v = (int32_t)(((int64_t)(mdeg - _angleMdeg) * 1000000LL) / dt);
      v /= 100;                                   // -> ddeg/s
      if (v >  32000) v =  32000;                 // clamp i16
      if (v < -32000) v = -32000;
      _velDdegS += (v - _velDdegS) / 4;           // LPF ringan
    }
  }
  _lastUpdateUs = now;
  _angleMdeg = (int32_t)mdeg;
  return true;
}

void AngleEncoder::setCenterAtCurrent() {
  _centerCounts = (int32_t)_totalCounts;
}

uint8_t AngleEncoder::typeCode() const { return 1; }

// ============================ QUADRATURE ===================================
#elif ENCODER_TYPE == ENCODER_QUAD

// Tabel decoding x4 (gray). Index = (prevState << 2) | nextState,
// state = (A << 1) | B. Urutan CW: 00 -> 01 -> 11 -> 10 -> 00.
static const int8_t QDEC_TABLE[16] = {
   0, +1, -1,  0,
  -1,  0,  0, +1,
  +1,  0,  0, -1,
   0, -1, +1,  0,
};

static volatile int32_t s_counts = 0;
static volatile uint8_t s_lastState = 0;

static void quadDecode() {
  uint8_t st = (digitalRead(PIN_QUAD_A) ? 2 : 0) | (digitalRead(PIN_QUAD_B) ? 1 : 0);
  s_counts += QDEC_TABLE[(s_lastState << 2) | st];
  s_lastState = st;
}

void quadInit() {
  pinMode(PIN_QUAD_A, INPUT_PULLUP);
  pinMode(PIN_QUAD_B, INPUT_PULLUP);
  s_lastState = (digitalRead(PIN_QUAD_A) ? 2 : 0) | (digitalRead(PIN_QUAD_B) ? 1 : 0);
  attachInterrupt(digitalPinToInterrupt(PIN_QUAD_A), quadDecode, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_QUAD_B), quadDecode, CHANGE);
}

int32_t quadReadCounts() {
  noInterrupts();
  int32_t c = s_counts;
  interrupts();
  return c;
}

bool AngleEncoder::begin() {
  quadInit();
  _centerCounts = quadReadCounts();
  return true;
}

bool AngleEncoder::update() {
  int32_t counts = quadReadCounts();
  int32_t rel = counts - _centerCounts;
  int64_t mdeg = ((int64_t)rel * 360000LL) / (int64_t)QUAD_CPR;
  if (_invert) mdeg = -mdeg;

  uint32_t now = micros();
  if (_lastUpdateUs) {
    uint32_t dt = now - _lastUpdateUs;
    if (dt >= 500 && dt < 100000UL) {
      int32_t v = (int32_t)(((int64_t)(mdeg - _angleMdeg) * 1000000LL) / dt);
      v /= 100;
      if (v >  32000) v =  32000;
      if (v < -32000) v = -32000;
      _velDdegS += (v - _velDdegS) / 4;
    }
  }
  _lastUpdateUs = now;
  _angleMdeg = (int32_t)mdeg;
  return true;
}

void AngleEncoder::setCenterAtCurrent() {
  _centerCounts = quadReadCounts();
}

uint8_t AngleEncoder::typeCode() const { return 2; }

// ============================ POTENSIOMETER ================================
#else

bool AngleEncoder::begin() {
  pinMode(PIN_POT, INPUT);
  analogRead(PIN_POT);   // buang pembacaan pertama
  return true;
}

static int32_t potMargin() {
  return ((int32_t)POT_MAX_RAW - (int32_t)POT_MIN_RAW) / 10;
}

bool AngleEncoder::update() {
  uint16_t raw = analogRead(PIN_POT);
  if (raw < POT_MIN_RAW) raw = POT_MIN_RAW;
  if (raw > POT_MAX_RAW) raw = POT_MAX_RAW;
  int32_t span = (int32_t)POT_MAX_RAW - POT_MIN_RAW;
  int32_t rel = (int32_t)raw - _centerRaw;
  int64_t mdeg = ((int64_t)rel * (int64_t)POT_SPAN_DEG * 1000LL) / span;
  if (_invert) mdeg = -mdeg;

  uint32_t now = micros();
  if (_lastUpdateUs) {
    uint32_t dt = now - _lastUpdateUs;
    if (dt >= 500 && dt < 100000UL) {
      int32_t v = (int32_t)(((int64_t)(mdeg - _angleMdeg) * 1000000LL) / dt);
      v /= 100;
      if (v >  32000) v =  32000;
      if (v < -32000) v = -32000;
      _velDdegS += (v - _velDdegS) / 4;
    }
  }
  _lastUpdateUs = now;
  _angleMdeg = (int32_t)mdeg;
  return true;
}

void AngleEncoder::setCenterAtCurrent() {
  uint16_t raw = analogRead(PIN_POT);
  // jaga center tetap dalam rentang linear
  if (raw < (uint16_t)(POT_MIN_RAW + potMargin())) raw = POT_MIN_RAW + potMargin();
  if (raw > (uint16_t)(POT_MAX_RAW - potMargin())) raw = POT_MAX_RAW - potMargin();
  _centerRaw = raw;
}

uint8_t AngleEncoder::typeCode() const { return 3; }

#endif
