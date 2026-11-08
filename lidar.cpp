#include <Arduino.h>
#include <Adafruit_VL53L0X.h>
#include "config.h"
#include "lidar.h"
#include "servo_control.h"
#include "leds.h"

void runAutoMode() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    int distance = measure.RangeMilliMeter;
    if (distance < TRACK_DISTANCE_MM) {
      int targetAngle = map(distance, 0, TRACK_DISTANCE_MM, 30, 150);
      targetAngle = constrain(targetAngle, 30, 150);
      if (targetAngle > currentAngle) currentAngle++;
      else if (targetAngle < currentAngle) currentAngle--;
      setServoAngle(HEAD_CHANNEL, currentAngle);
      setColor(0, 255, 0);
    } else {
      setColor(0, 0, 255);
      returnToCenter();
    }
  } else {
    setColor(0, 0, 255);
    returnToCenter();
  }
}

void returnToCenter() {
  if (currentAngle > 90) { currentAngle--; setServoAngle(HEAD_CHANNEL, currentAngle); }
  if (currentAngle < 90) { currentAngle++; setServoAngle(HEAD_CHANNEL, currentAngle); }
}
