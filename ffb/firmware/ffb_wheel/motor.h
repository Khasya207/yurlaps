#pragma once
// ============================================================================
//  YurFFB — abstraksi driver motor (BTS7960 2-pwm / PWM+DIR)
//  Nilai torsi dalam "mili-persen" (mPct): -1000..+1000 = -100%..+100% duty.
//  Tanda positif = mendorong sudut ke arah positif (+).
// ============================================================================
#include <Arduino.h>
#include "config.h"

class MotorDriver {
public:
  void begin();
  void setEnable(bool on);
  // keluarkan torsi (sudah dibatasi & di-slew oleh control). mPct -1000..1000
  void setTorque(int16_t mPct);
  // matikan semua output (coast). Dipanggil saat fault/e-stop.
  void stop();
  bool isEnabled() const { return _enabled; }

private:
  void _write(int16_t duty);       // duty 0..(PWM_TOP) ke hardware
  bool _enabled = false;
  int16_t _lastDuty = 0;
};
