#include "motor.h"

// ---------------------------------------------------------------------------
// Setup PWM 20 kHz (khusus ATmega328P / ATmega32U4, pin 9 & 10 = Timer1).
// Mode 14 (Fast PWM, TOP=ICR1), tanpa prescaler -> 16 MHz / (ICR1+1).
// ICR1 = 799 -> 20 kHz, resolusi 800 step (lebih dari cukup utk FFB).
// Untuk DRIVER_PWM_DIR, channel B dipakai sebagai GPIO arah (bukan PWM).
// ---------------------------------------------------------------------------
#if PWM_20KHZ && (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega32U4__))
#define PWM_TOP_20K 799
static void setupMotorPwm() {
  pinMode(PIN_MOTOR_PWM_A, OUTPUT);
  pinMode(PIN_MOTOR_PWM_B, OUTPUT);
  analogWrite(PIN_MOTOR_PWM_A, 0);
  analogWrite(PIN_MOTOR_PWM_B, 0);
  TCCR1A = _BV(WGM11);                       // mode 14 (WGM11..13), channel A: COM1A1 di bawah
#if DRIVER_TYPE == DRIVER_BTS7960
  TCCR1A |= _BV(COM1A1) | _BV(COM1B1);       // kedua channel = PWM non-inverting
#else
  TCCR1A |= _BV(COM1A1);                     // hanya channel A = PWM; B = GPIO DIR
#endif
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1   = PWM_TOP_20K;                      // 16 MHz / 800 = 20 kHz
  OCR1A  = 0;
  OCR1B  = 0;
}
static inline void pwmWriteA(uint16_t d) { OCR1A = d; }
static inline void pwmWriteB(uint16_t d) { OCR1B = d; }
static const uint16_t PWM_TOP = PWM_TOP_20K;
#else
// Fallback: analogWrite biasa (0..255) — Uno non-20k, Mega, ESP32, dll.
static void setupMotorPwm() {
  pinMode(PIN_MOTOR_PWM_A, OUTPUT);
  pinMode(PIN_MOTOR_PWM_B, OUTPUT);
  analogWrite(PIN_MOTOR_PWM_A, 0);
  analogWrite(PIN_MOTOR_PWM_B, 0);
}
static inline void pwmWriteA(uint16_t d) { analogWrite(PIN_MOTOR_PWM_A, (uint8_t)(d >> 2)); }
static inline void pwmWriteB(uint16_t d) { analogWrite(PIN_MOTOR_PWM_B, (uint8_t)(d >> 2)); }
static const uint16_t PWM_TOP = 1023;
#endif

void MotorDriver::begin() {
  setupMotorPwm();
  pinMode(PIN_MOTOR_EN, OUTPUT);
  digitalWrite(PIN_MOTOR_EN, LOW);
  _enabled = false;
  _lastDuty = 0;
}

void MotorDriver::setEnable(bool on) {
  _enabled = on;
  digitalWrite(PIN_MOTOR_EN, on ? HIGH : LOW);
  if (!on) stop();
}

void MotorDriver::stop() {
#if DRIVER_TYPE == DRIVER_PWM_DIR
  pwmWriteA(0);              // duty 0 = motor bebas/coast
#else
  pwmWriteA(0);
  pwmWriteB(0);
#endif
  _lastDuty = 0;
}

void MotorDriver::_write(int16_t duty) {
  // duty: 0..PWM_TOP, tanda = arah
  if (duty > (int16_t)PWM_TOP)  duty = (int16_t)PWM_TOP;
  if (duty < -(int16_t)PWM_TOP) duty = -(int16_t)PWM_TOP;

#if DRIVER_TYPE == DRIVER_BTS7960
  // RPWM/LPWM: satu pin PWM aktif, satunya 0. (0,0) = coast.
  if (duty >= 0) { pwmWriteA((uint16_t)duty); pwmWriteB(0); }
  else           { pwmWriteA(0); pwmWriteB((uint16_t)(-duty)); }
#elif DRIVER_TYPE == DRIVER_PWM_DIR
  // PWM + DIR (Cytron MD10C/MD13S, DRV8871 dgn mode in/PH): arah = pin B
  digitalWrite(PIN_MOTOR_PWM_B, duty >= 0 ? LOW : HIGH);
  pwmWriteA((uint16_t)(duty >= 0 ? duty : -duty));
#endif
  _lastDuty = duty;
}

void MotorDriver::setTorque(int16_t mPct) {
  if (!_enabled) { stop(); return; }
  if (mPct >  ABS_MAX_TORQUE) mPct =  ABS_MAX_TORQUE;
  if (mPct < -ABS_MAX_TORQUE) mPct = -ABS_MAX_TORQUE;
  // mPct (-1000..1000) -> duty (0..PWM_TOP)
  int32_t d = ((int32_t)mPct * (int32_t)PWM_TOP) / 1000;
  _write((int16_t)d);
}
