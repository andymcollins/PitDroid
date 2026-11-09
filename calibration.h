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
extern const char CAL_HTML[];
