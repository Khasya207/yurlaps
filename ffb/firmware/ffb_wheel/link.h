#pragma once
// ============================================================================
//  YurFFB — framing serial (lihat protocol.h untuk format frame).
//  RX: state machine byte-per-byte, CRC diperiksa, callback per frame valid.
//  TX: non-blocking — frame dibuang bila buffer TX penuh (telemetri boleh
//      hilang, perintah akan datang lagi; jangan pernah block loop kontrol).
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "protocol.h"

typedef void (*FrameHandler)(uint8_t cmd, const uint8_t *payload, uint8_t payloadLen);

class SerialLink {
public:
  void begin(FrameHandler handler);
  void poll();                       // panggil sesering mungkin dari loop()

  void sendFrame(uint8_t cmd, const uint8_t *payload, uint8_t payloadLen);
  void sendU8(uint8_t cmd, uint8_t v);
  void sendI16(uint8_t cmd, int16_t v);
  void sendU16(uint8_t cmd, uint16_t v);
  void sendLog(const char *msg);
  uint32_t lastRxMs() const { return _lastRxMs; }
  uint32_t goodFrames() const { return _goodFrames; }
  uint32_t badFrames() const { return _badFrames; }

  static uint8_t crc8(const uint8_t *data, uint8_t len);

private:
  enum RxState : uint8_t { RX_WAIT_SOF1, RX_WAIT_SOF2, RX_LEN, RX_BODY };
  void handleByte(uint8_t b);

  FrameHandler _handler = nullptr;
  RxState   _state = RX_WAIT_SOF1;
  uint8_t   _len = 0;
  uint8_t   _pos = 0;
  uint8_t   _buf[YURFFB_MAX_LEN + 2];   // [cmd][payload...][crc]
  uint32_t  _lastRxMs = 0;
  uint32_t  _goodFrames = 0;
  uint32_t  _badFrames = 0;
};
