// Mock Wire (I2C) — lihat Arduino.h di folder ini.
#pragma once
#include <stdint.h>
#include <deque>

class MockWire {
public:
  void begin() {}
  void begin(int, int) {}
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t) { txRegSet = false; }
  size_t write(uint8_t v) { lastReg = v; txRegSet = true; return 1; }
  uint8_t endTransmission(bool) { return nack ? 1 : 0; }   // 1 = NACK/bus error
  uint8_t endTransmission() { return nack ? 1 : 0; }

  uint8_t requestFrom(uint8_t addr, uint8_t n) {
    (void)addr;
    rxBuf.clear();
    if (nack || n < 2) return 0;
    rxBuf.push_back((g_rawAngle >> 8) & 0x0F);   // AS5600: hi byte = bit 11..8
    rxBuf.push_back(g_rawAngle & 0xFF);
    return 2;
  }
  int read() {
    if (rxBuf.empty()) return 0;
    uint8_t v = rxBuf.front();
    rxBuf.pop_front();
    return v;
  }

  // helper test
  uint16_t g_rawAngle = 0;      // sudut mentah 12-bit yang "dilihat" encoder
  bool     nack = false;        // true = simulasikan device tidak menjawab
  std::deque<uint8_t> rxBuf;
  uint8_t  lastReg = 0;
  bool     txRegSet = false;
};
extern MockWire Wire;
