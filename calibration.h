#pragma once
#include <Arduino.h>

struct ServoCal {
  int minPwm;
  int maxPwm;
  int centerPwm;
};

void calibrationBegin();
ServoCal calibrationGet(uint8_t ch);
void calibrationSave(uint8_t ch, int minPwm, int maxPwm, int centerPwm);
void calibrationHandler();

// Non-blocking test sweep started by /cal/sweep; call from loop().
void calibrationSweepStart(uint8_t ch);
void calibrationUpdate();
bool calibrationSweeping();

extern const char CAL_HTML[];
