#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "servo_control.h"

void setServoAngle(uint8_t channel, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(channel, 0, pulse);
}

void startupAnimation() {
  for (int a = 90; a <= 140; a += 2) { setServoAngle(HEAD_CHANNEL, a); delay(15); }
  for (int a = 140; a >= 40;  a -= 2) { setServoAngle(HEAD_CHANNEL, a); delay(15); }
  for (int a = 40;  a <= 90;  a += 2) { setServoAngle(HEAD_CHANNEL, a); delay(15); }
  currentAngle = 90;

  extern Adafruit_NeoPixel pixels;
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
