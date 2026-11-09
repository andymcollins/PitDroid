#include <Arduino.h>
#include "config.h"
#include "radar.h"

// RD-03D frame parser — adjust byte offsets to your firmware's frame format.
static uint8_t buf[64];
static uint8_t idx = 0;

void radarBegin() {
  delay(1000); // radar stabilise
}

RadarTarget radarRead() {
  RadarTarget t = {false, 0, 0.0f};
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    if (idx < sizeof(buf)) buf[idx++] = b;
    if (b == 0x55 && idx > 6) {
      int dist = (buf[2] << 8) | buf[3];
      int8_t ang = (int8_t)buf[4];
      if (dist > 0 && dist < 8000) {
        t.valid = true;
        t.distanceMm = dist;
        t.angleDeg = (float)ang;
      }
      idx = 0;
    }
    if (idx >= sizeof(buf)) idx = 0;
  }
  return t;
}
