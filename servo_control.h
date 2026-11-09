#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

extern Adafruit_PWMServoDriver pwm;
void setServoAngle(uint8_t channel, int angle);
void setServoPwm(uint8_t channel, int pwm);
void startupAnimation();
