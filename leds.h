#pragma once
#include <stdint.h>
extern Adafruit_NeoPixel pixels;
void setColor(uint8_t r, uint8_t g, uint8_t b);
uint32_t Wheel(byte WheelPos);
