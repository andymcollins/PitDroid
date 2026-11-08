#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "leds.h"

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < pixels.numPixels(); i++) pixels.setPixelColor(i, pixels.Color(r, g, b));
  pixels.show();
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)  return pixels.Color(255 - WheelPos*3, 0, WheelPos*3);
  if (WheelPos < 170) { WheelPos -= 85; return pixels.Color(0, WheelPos*3, 255 - WheelPos*3); }
  WheelPos -= 170;
  return pixels.Color(WheelPos*3, 255 - WheelPos*3, 0);
}
