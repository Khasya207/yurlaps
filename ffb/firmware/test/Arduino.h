// ============================================================================
//  Mock Arduino API untuk compile & unit-test firmware YurFFB di PC (g++).
//  Hanya dipakai oleh ffb/firmware/test — BUKAN bagian build Arduino.
//  Jalankan: make -C ffb/firmware/test
// ============================================================================
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <deque>
#include <vector>

// ---------- stub register Timer1 (jalur PWM 20 kHz, ATmega328P) -----------
extern uint8_t  TCCR1A, TCCR1B;
extern uint16_t ICR1, OCR1A, OCR1B;
#define _BV(x)   (1 << (x))
#define COM1A1   7
#define COM1B1   5
#define WGM11    1
#define WGM12    3
#define WGM13    4
#define CS10     0

// ---------- konstanta dasar ----------
#define HIGH 1
#define LOW  0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 1
#define A0 14

typedef uint8_t byte;

// ---------- waktu (dikendalikan test) ----------
extern unsigned long g_mockMillis, g_mockMicros;
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);

// ---------- digital/analog IO (state dikendalikan test) ----------
extern uint8_t  g_mockPinModes[32];
extern uint8_t  g_mockPinState[32];
extern int      g_mockAnalogValue[8];

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
int  analogRead(uint8_t pin);
void attachInterrupt(uint8_t irq, void (*isr)(), int mode);
int  digitalPinToInterrupt(uint8_t pin);
extern void (*g_mockIsr[3])();   // irq 0 -> pin2, irq 1 -> pin3
inline void noInterrupts() {}    // host test single-threaded
inline void interrupts()   {}

// ---------- Serial ----------
class MockSerial {
public:
  void begin(unsigned long) {}
  int  available() { return (int)rxBuf.size(); }
  int  read();
  size_t write(uint8_t b);
  size_t write(const uint8_t *buf, size_t n);
  int  availableForWrite() { return 128; }

  // helper test
  std::deque<uint8_t> rxBuf;
  std::vector<uint8_t> txBytes;
  void feed(const uint8_t *b, size_t n) { for (size_t i = 0; i < n; i++) rxBuf.push_back(b[i]); }
  void clearTx() { txBytes.clear(); }
};
extern MockSerial Serial;
