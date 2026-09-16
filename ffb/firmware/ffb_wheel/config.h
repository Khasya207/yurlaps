#pragma once
// ============================================================================
//  YurFFB — konfigurasi perangkat keras & parameter kontrol
//  Ubah bagian ini sesuai hardware kamu, TIDAK perlu menyentuh file lain.
// ============================================================================

// ---------------------------------------------------------------------------
// 1. PILIH JENIS ENCODER  (pilih SATU)
// ---------------------------------------------------------------------------
#define ENCODER_AS5600   1  // encoder magnetik I2C 12-bit (paling direkomendasikan)
#define ENCODER_QUAD     2  // encoder quadrature A/B (motor servo/DC dgn encoder)
#define ENCODER_POT      3  // potensiometer di poros kemudi (paling murah)

#ifndef ENCODER_TYPE
#define ENCODER_TYPE     ENCODER_AS5600
#endif

// ---------------------------------------------------------------------------
// 2. PILIH JENIS DRIVER MOTOR (pilih SATU)
// ---------------------------------------------------------------------------
#define DRIVER_BTS7960   1  // BTS7960 / IBT-2: dua pin PWM (RPWM, LPWM) + EN
#define DRIVER_PWM_DIR   2  // Cytron MD10C/MD13S, DRV8871 (mode PWM+DIR), L298 dgn jumper

#ifndef DRIVER_TYPE
#define DRIVER_TYPE      DRIVER_BTS7960
#endif

// ---------------------------------------------------------------------------
// 3. PIN — default per board. Sesuaikan bila perlu.
// ---------------------------------------------------------------------------
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(__AVR_ATmega328P__)
  // Arduino Uno / Nano (ATmega328P)
  // PWM 20 kHz memakai Timer1 -> WAJIB pin 9 & 10 untuk RPWM/LPWM.
  #define PIN_MOTOR_PWM_A   9    // RPWM (BTS7960) atau PWM (PWM_DIR)
  #define PIN_MOTOR_PWM_B   10   // LPWM (BTS7960) atau DIR (PWM_DIR)
  #define PIN_MOTOR_EN      8    // EN (BTS7960, hubungkan R_EN & L_EN ke sini)
  #define PIN_QUAD_A        2    // interrupt
  #define PIN_QUAD_B        3    // interrupt
  #define PIN_POT           A0
  #define PIN_STATUS_LED    13
  // I2C (AS5600): A4=SDA, A5=SCL (fix di ATmega328P)

#elif defined(__AVR_ATmega32U4__)
  // Arduino Leonardo / Pro Micro (ATmega32U4). Serial = USB CDC (baud diabaikan).
  #define PIN_MOTOR_PWM_A   9    // Timer1 20 kHz -> pin 9 & 10
  #define PIN_MOTOR_PWM_B   10
  #define PIN_MOTOR_EN      8
  #define PIN_QUAD_A        2
  #define PIN_QUAD_B        3
  #define PIN_POT           A0
  #define PIN_STATUS_LED    17   // RX_LED biar D13 bebas; ganti 13 kalau mau

#elif defined(__AVR_ATmega2560__)
  // Arduino Mega — PWM 20 kHz hanya diimplementasikan utk 328P/32U4,
  // di Mega dipakai analogWrite biasa -> gunakan pin PWM bebas (2..13).
  #define PIN_MOTOR_PWM_A   5
  #define PIN_MOTOR_PWM_B   6
  #define PIN_MOTOR_EN      8
  #define PIN_QUAD_A        2
  #define PIN_QUAD_B        3
  #define PIN_POT           A0
  #define PIN_STATUS_LED    13

#elif defined(ARDUINO_ARCH_ESP32)
  // ESP32 (termasuk C3 Super Mini). Hindari strapping pin di C3 (2,8,9).
  // C3 Super Mini aman: 0,1,3,4,5,6,7,10,20,21
  #define PIN_MOTOR_PWM_A   4
  #define PIN_MOTOR_PWM_B   5
  #define PIN_MOTOR_EN      6
  #define PIN_QUAD_A        1
  #define PIN_QUAD_B        3
  #define PIN_POT           0    // ADC0 (GPIO0)
  #define PIN_STATUS_LED    10
  #define PIN_I2C_SDA       7    // khusus ESP32 (begin(SDA,SCL))
  #define PIN_I2C_SCL       10
  #define SERIAL_BAUD       115200
#endif

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

// ---------------------------------------------------------------------------
// 4. PARAMETER KONTROL (default; bisa diubah runtime lewat aplikasi)
// ---------------------------------------------------------------------------
#define CTRL_PERIOD_US     1000   // periode loop kontrol: 1000 us = 1 kHz
#define WATCHDOG_MS        200    // host diam > ini -> torsi host dipaksa 0
#define TELEM_PERIOD_MS    3      // telemetri default ~333 Hz
#define I2C_CLOCK_HZ       400000 // AS5600 mendukung 400 kHz

// Batas fisik JANGAN dilewati:
#define ABS_MAX_TORQUE     1000   // 100% duty, hard limit di firmware

// Encoder AS5600
#define AS5600_ADDR        0x36
#define AS5600_REG_ANGLE_H 0x0C

// Encoder quadrature: resolusi HITUNGAN per putaran penuh
// (PPR encoder x 4 untuk decoding x4 kita). Contoh: encoder 600 PPR -> 2400.
#define QUAD_CPR           2400

// Potensiometer: rentang raw & sudut yang dipetakan linear
#define POT_MIN_RAW        30
#define POT_MAX_RAW        993
#define POT_SPAN_DEG       300    // rentang mekanik pot (derajat)

// ---------------------------------------------------------------------------
// 5. OPSI
// ---------------------------------------------------------------------------
#define PWM_20KHZ          1      // 1 = PWM 20 kHz (senyap) di 328P/32U4 pin 9/10
                                  // 0 = analogWrite biasa (~490/980 Hz, ada dengung)
#define DEMO_SELF_TEST     0      // 1 = tanpa host: pegas center sederhana utk tes
#define DEBUG_FRAMES       0      // 1 = log frame mentah ke Serial (rawan timing!)
