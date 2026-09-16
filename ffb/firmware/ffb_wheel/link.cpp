#include "link.h"

// CRC-8: poly 0x07, init 0x00, no reflect, no xorout (identik dgn sisi aplikasi)
uint8_t SerialLink::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

void SerialLink::begin(FrameHandler handler) {
  _handler = handler;
  Serial.begin(SERIAL_BAUD);
}

void SerialLink::poll() {
  while (Serial.available() > 0) {
    handleByte((uint8_t)Serial.read());
  }
}

void SerialLink::handleByte(uint8_t b) {
  switch (_state) {
    case RX_WAIT_SOF1:
      if (b == YURFFB_SOF1) _state = RX_WAIT_SOF2;
      break;

    case RX_WAIT_SOF2:
      if (b == YURFFB_SOF2) _state = RX_LEN;
      else if (b != YURFFB_SOF1) _state = RX_WAIT_SOF1;  // 0xAA lagi? tetap di sini
      break;

    case RX_LEN:
      if (b < 2 || b > YURFFB_MAX_LEN) { _state = RX_WAIT_SOF1; break; }
      _len = b;
      _pos = 0;
      _state = RX_BODY;
      break;

    case RX_BODY:
      _buf[_pos++] = b;
      if (_pos >= _len) {
        // frame lengkap: [cmd][payload...][crc]
        uint8_t crc = crc8(_buf, _len - 1);
        if (crc == _buf[_len - 1]) {
          _goodFrames++;
          _lastRxMs = millis();
          if (_handler) _handler(_buf[0], &_buf[1], (uint8_t)(_len - 2));
        } else {
          _badFrames++;
        }
        _state = RX_WAIT_SOF1;
      }
      break;
  }
}

void SerialLink::sendFrame(uint8_t cmd, const uint8_t *payload, uint8_t payloadLen) {
  uint8_t len = (uint8_t)(payloadLen + 2);   // cmd + payload + crc
  if (len > YURFFB_MAX_LEN) return;
  // jangan pernah block loop kontrol: kalau buffer hampir penuh, buang frame
  if (Serial.availableForWrite() < (int)(len + 3)) return;

  uint8_t body[YURFFB_MAX_LEN];
  body[0] = cmd;
  if (payloadLen) memcpy(&body[1], payload, payloadLen);
  uint8_t crc = crc8(body, (uint8_t)(len - 1));

  Serial.write(YURFFB_SOF1);
  Serial.write(YURFFB_SOF2);
  Serial.write(len);
  Serial.write(body, len - 1);
  Serial.write(crc);
}

void SerialLink::sendU8(uint8_t cmd, uint8_t v)  { sendFrame(cmd, &v, 1); }
void SerialLink::sendI16(uint8_t cmd, int16_t v) { sendFrame(cmd, (const uint8_t *)&v, 2); }
void SerialLink::sendU16(uint8_t cmd, uint16_t v){ sendFrame(cmd, (const uint8_t *)&v, 2); }

void SerialLink::sendLog(const char *msg) {
  uint8_t buf[48];
  uint8_t n = 0;
  while (msg[n] && n < 47) { buf[n] = (uint8_t)msg[n]; n++; }
  buf[n] = 0;
  sendFrame(YURFFB_MSG_LOG, buf, (uint8_t)(n + 1));
}
