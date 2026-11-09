#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "servo_control.h"
#include "calibration.h"
#include "leds.h"

void setServoAngle(uint8_t channel, int angle) {
  angle = constrain(angle, 0, 180);
  ServoCal c = calibrationGet(channel);
  int pulse = map(angle, 0, 180, c.minPwm, c.maxPwm);
  pwm.setPWM(channel, 0, pulse);
}

void setServoPwm(uint8_t channel, int pwmVal) {
  pwm.setPWM(channel, 0, constrain(pwmVal, 100, 700));
}

void startupAnimation() {
  ServoCal pan  = calibrationGet(CH_PAN);
  ServoCal tilt = calibrationGet(CH_TILT);
  ServoCal armL = calibrationGet(CH_ARM_L);
  ServoCal armR = calibrationGet(CH_ARM_R);
  ServoCal hip  = calibrationGet(CH_HIP);

  int panMid  = (pan.minPwm  + pan.maxPwm)  / 2;
  int tiltMid = (tilt.minPwm + tilt.maxPwm) / 2;
  int armMid  = (armL.minPwm + armL.maxPwm) / 2;
  int hipMid  = (hip.minPwm  + hip.maxPwm)  / 2;

  for (int v = panMid; v <= pan.maxPwm; v += 5) { pwm.setPWM(CH_PAN, 0, v); delay(10); }
  for (int v = pan.maxPwm; v >= pan.minPwm; v -= 5) { pwm.setPWM(CH_PAN, 0, v); delay(10); }
  for (int v = pan.minPwm; v <= panMid; v += 5) { pwm.setPWM(CH_PAN, 0, v); delay(10); }

  for (int v = armMid; v <= armL.maxPwm; v += 5) {
    pwm.setPWM(CH_ARM_L, 0, v);
    pwm.setPWM(CH_ARM_R, 0, v);
    delay(10);
  }
  for (int v = armL.maxPwm; v >= armMid; v -= 5) {
    pwm.setPWM(CH_ARM_L, 0, v);
    pwm.setPWM(CH_ARM_R, 0, v);
    delay(10);
  }

  pwm.setPWM(CH_PAN,   0, panMid);
  pwm.setPWM(CH_TILT,  0, tiltMid);
  pwm.setPWM(CH_ARM_L, 0, armMid);
  pwm.setPWM(CH_ARM_R, 0, armMid);
  pwm.setPWM(CH_HIP,   0, hipMid);

  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < pixels.numPixels(); i++) {
      uint8_t w = (i * 256 / pixels.numPixels() + j * 85) & 255;
      uint8_t p = 255 - w;
      uint32_t c;
      if (p < 85)       c = pixels.Color(255 - p*3, 0, p*3);
      else if (p < 170) { p -= 85; c = pixels.Color(0, p*3, 255 - p*3); }
      else              { p -= 170; c = pixels.Color(p*3, 255 - p*3, 0); }
      pixels.setPixelColor(i, c);
    }
    pixels.show();
    delay(120);
  }
  pixels.clear();
  pixels.show();
}
