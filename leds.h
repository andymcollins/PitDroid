#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel pixels;

enum LedMode : uint8_t {
  LED_SOLID = 0,
  LED_RAINBOW,
  LED_CHASE,
  LED_COMET,
  LED_BREATHE,
  LED_SPARKLE,
  LED_SCANNER,
  LED_THEATER,
  LED_RINGS,
  LED_MODE_COUNT
};

void ledBegin();
void ledUpdate();

// Manual control (sets the engine to manual, so radar status won't override it)
void ledSetColor(uint8_t r, uint8_t g, uint8_t b);
void ledSetMode(uint8_t mode);
void ledSetSpeed(uint8_t speed);          // 1 = slow, 100 = fast
void ledSetBrightness(uint8_t brightness);

// Status control (only applied while the user hasn't taken manual control)
void ledSetStatus(uint8_t r, uint8_t g, uint8_t b);
void ledAuto();

// Legacy helper kept for existing callers
void setColor(uint8_t r, uint8_t g, uint8_t b);
uint32_t Wheel(byte WheelPos);
