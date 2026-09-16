#pragma once
// ============================================================================
//  YurFFB — abstraksi encoder sudut.
//  Tiga jenis didukung (pilih di config.h):
//    ENCODER_AS5600 : magnetic encoder I2C 12-bit, absolut + tracking multi-turn
//    ENCODER_QUAD   : quadrature A/B via interrupt (decoding x4)
//    ENCODER_POT    : potensiometer linear
//  Keluarkan sudut dalam mili-derajat (mdeg), multi-turn, relatif ke titik
//  tengah yang diset lewat setCenterAtCurrent().
// ============================================================================
#include <Arduino.h>
#include "config.h"

class AngleEncoder {
public:
  bool begin();
  // Baca & update akumulasi sudut. Panggil SEKALI per siklus kontrol.
  // Return false kalau encoder fault (kabel lepas / I2C mati).
  bool update();
  int32_t angleMdeg() const { return _angleMdeg; }   // multi-turn, relatif center
  int32_t velocityDdegS() const { return _velDdegS; } // ddeg/s (deci-derajat/detik)
  void setCenterAtCurrent();
  void setInvert(bool inv) { _invert = inv; }
  uint8_t typeCode() const;

private:
#if ENCODER_TYPE == ENCODER_AS5600
  bool readRaw(uint16_t &raw);
  int64_t  _totalCounts = 0;   // akumulasi 12-bit unwrap
  uint16_t _lastRaw     = 0;
  bool     _lastRawValid = false;
  int32_t  _centerCounts = 0;  // center dalam satuan counts
  uint8_t  _i2cFaults = 0;
#elif ENCODER_TYPE == ENCODER_QUAD
  int32_t _centerCounts = 0;
#else
  int32_t _centerRaw = (POT_MIN_RAW + POT_MAX_RAW) / 2;
#endif
  int32_t _angleMdeg = 0;
  int32_t _velDdegS  = 0;
  bool    _invert = false;
  uint32_t _lastUpdateUs = 0;
};

// dipakai implementasi QUAD di .cpp
#if ENCODER_TYPE == ENCODER_QUAD
void quadInit();
int32_t quadReadCounts();
#endif
