#pragma once
#include <stdint.h>
extern Adafruit_PWMServoDriver pwm;
void setServoAngle(uint8_t channel, int angle);
void startupAnimation();
