#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "servo_control.h"
#include "calibration.h"
#include "leds.h"

// Angle -> pulse uses the calibrated centre, not the midpoint of min/max, so a
// servo whose mechanical centre is off-centre still sits straight at 90 deg.
static int angleToPulse(int angle, const ServoCal& c) {
  angle = constrain(angle, 0, 180);
  if (angle <= 90) return map(angle, 0, 90, c.minPwm, c.centerPwm);
  return map(angle, 90, 180, c.centerPwm, c.maxPwm);
}

void setServoAngle(uint8_t channel, int angle) {
  ServoCal c = calibrationGet(channel);
  pwm.setPWM(channel, 0, angleToPulse(angle, c));
}

void setServoPwm(uint8_t channel, int pwmVal) {
  pwm.setPWM(channel, 0, constrain(pwmVal, 100, 700));
}

static int calMid(uint8_t ch) {
  ServoCal c = calibrationGet(ch);
  return (c.minPwm + c.maxPwm) / 2;
}

// ---------------------------------------------------------------------------
// Startup animation, non-blocking state machine driven from loop().
// ---------------------------------------------------------------------------
static uint8_t  animStage    = 0;
static int      animValue    = 0;
static uint32_t animLastMs   = 0;
static uint8_t  animLedFrame = 0;

void startupAnimationBegin() {
  if (animStage) return;              // already running
  animValue = calMid(CH_PAN);
  animStage = 1;
  animLastMs = millis();
  animLedFrame = 0;
  if (SERVO_PRESENT(CH_PAN)) setServoPwm(CH_PAN, animValue);
}

void startupAnimationUpdate() {
  if (!animStage) return;

  uint32_t now = millis();
  ServoCal pan  = calibrationGet(CH_PAN);
  ServoCal armL = calibrationGet(CH_ARM_L);
  int panMid  = (pan.minPwm + pan.maxPwm) / 2;
  int armMid  = (armL.minPwm + armL.maxPwm) / 2;

  switch (animStage) {
    case 1:  // pan out
      if (now - animLastMs < 10) return;
      animLastMs = now;
      animValue += 5;
      if (animValue >= pan.maxPwm) { animValue = pan.maxPwm; animStage = 2; }
      if (SERVO_PRESENT(CH_PAN)) setServoPwm(CH_PAN, animValue);
      break;

    case 2:  // pan in
      if (now - animLastMs < 10) return;
      animLastMs = now;
      animValue -= 5;
      if (animValue <= pan.minPwm) { animValue = pan.minPwm; animStage = 3; }
      if (SERVO_PRESENT(CH_PAN)) setServoPwm(CH_PAN, animValue);
      break;

    case 3:  // pan home
      if (now - animLastMs < 10) return;
      animLastMs = now;
      animValue += 5;
      if (animValue >= panMid) {
        if (SERVO_PRESENT(CH_PAN)) setServoPwm(CH_PAN, panMid);
        animValue = armMid;
        animStage = 4;
        break;
      }
      if (SERVO_PRESENT(CH_PAN)) setServoPwm(CH_PAN, animValue);
      break;

    case 4:  // arms out
      if (!(SERVO_PRESENT(CH_ARM_L) && SERVO_PRESENT(CH_ARM_R))) { animStage = 6; break; }
      if (now - animLastMs < 10) return;
      animLastMs = now;
      animValue += 5;
      if (animValue >= armL.maxPwm) { animValue = armL.maxPwm; animStage = 5; }
      setServoPwm(CH_ARM_L, animValue);
      setServoPwm(CH_ARM_R, animValue);
      break;

    case 5:  // arms home
      if (now - animLastMs < 10) return;
      animLastMs = now;
      animValue -= 5;
      if (animValue <= armMid) { animValue = armMid; animStage = 6; }
      setServoPwm(CH_ARM_L, animValue);
      setServoPwm(CH_ARM_R, animValue);
      break;

    case 6:  // LED swirl
      if (now - animLastMs < 120) return;
      animLastMs = now;
      if (animLedFrame >= 3) { animStage = 7; break; }
      for (int i = 0; i < pixels.numPixels(); i++) {
        uint8_t w = (i * 256 / pixels.numPixels() + animLedFrame * 85) & 255;
        uint8_t p = 255 - w;
        uint32_t c;
        if (p < 85)        c = pixels.Color(255 - p * 3, 0, p * 3);
        else if (p < 170)  { p -= 85; c = pixels.Color(0, p * 3, 255 - p * 3); }
        else               { p -= 170; c = pixels.Color(p * 3, 255 - p * 3, 0); }
        pixels.setPixelColor(i, c);
      }
      pixels.show();
      animLedFrame++;
      break;

    case 7:  // home everything and finish
      pixels.clear();
      pixels.show();
      if (SERVO_PRESENT(CH_PAN))   setServoPwm(CH_PAN,   panMid);
      if (SERVO_PRESENT(CH_TILT))  setServoPwm(CH_TILT,  calMid(CH_TILT));
      if (SERVO_PRESENT(CH_ARM_L)) setServoPwm(CH_ARM_L, armMid);
      if (SERVO_PRESENT(CH_ARM_R)) setServoPwm(CH_ARM_R, armMid);
      if (SERVO_PRESENT(CH_HIP))   setServoPwm(CH_HIP,   calMid(CH_HIP));
      animStage = 0;
      break;
  }
}

bool startupAnimationRunning() {
  return animStage != 0;
}
