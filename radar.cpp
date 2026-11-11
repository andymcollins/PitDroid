#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "radar.h"

// RD-03D multi-target tracking frame:
//   header  AA FF 03 00
//   3 x 8 bytes: x (int16 LE), y (int16 LE), speed (int16 LE), resolution (uint16 LE)
//   tail    55 CC
// x/y are millimetres; x is lateral, y is forward. A target of 0,0 is empty.
static const uint8_t  FRAME_LEN   = 4 + 3 * 8 + 2;
static const uint8_t  PAYLOAD_OFF = 4;

static uint8_t frame[FRAME_LEN];
static uint8_t len = 0;

static inline int16_t le16(const uint8_t* p) {
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline void dropFirstByte() {
  for (uint8_t i = 1; i < len; i++) frame[i - 1] = frame[i];
  if (len) len--;
}

void radarBegin() {
  while (Serial2.available()) Serial2.read();
  len = 0;
  delay(1000);  // radar stabilise
}

RadarTarget radarRead() {
  RadarTarget best = {false, 0, 0.0f};
  float bestDist = 1e9f;

  while (Serial2.available()) {
    if (len < sizeof(frame)) frame[len++] = (uint8_t)Serial2.read();

    // Resync on a header that does not line up.
    while (len > 0 && len < 4 &&
           !(frame[0] == 0xAA &&
             (len < 2 || frame[1] == 0xFF) &&
             (len < 3 || frame[2] == 0x03) &&
             (len < 4 || frame[3] == 0x00))) {
      dropFirstByte();
    }
    if (len >= 4 &&
        !(frame[0] == 0xAA && frame[1] == 0xFF && frame[2] == 0x03 && frame[3] == 0x00)) {
      dropFirstByte();
      continue;
    }

    if (len < FRAME_LEN) continue;

    if (frame[FRAME_LEN - 2] != 0x55 || frame[FRAME_LEN - 1] != 0xCC) {
      dropFirstByte();
      continue;
    }

    for (uint8_t i = 0; i < 3; i++) {
      const uint8_t* p = frame + PAYLOAD_OFF + i * 8;
      int16_t x = le16(p);
      int16_t y = le16(p + 2);
      if (x == 0 && y == 0) continue;  // empty slot

      float dist = sqrtf((float)x * (float)x + (float)y * (float)y);
      if (dist < 1.0f || dist > 8000.0f) continue;

      if (dist < bestDist) {
        bestDist = dist;
        best.valid = true;
        best.distanceMm = (int)dist;
        best.angleDeg = atan2f((float)x, (float)y) * 180.0f / PI;
      }
    }

    len = 0;
  }

  return best;
}
