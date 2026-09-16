// Mock EEPROM — lihat Arduino.h di folder ini.
#pragma once
#include <stdint.h>

class MockEeprom {
public:
  void begin(size_t) {}
  uint8_t read(int addr) { return mem[addr & 1023]; }
  void write(int addr, uint8_t v) { mem[addr & 1023] = v; dirty = true; }
  void commit() { dirty = false; }

  uint8_t mem[1024] = {0};
  bool dirty = false;
};
extern MockEeprom EEPROM;
