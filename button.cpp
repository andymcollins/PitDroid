#include <Arduino.h>
#include "config.h"
#include "button.h"
#include "servo_control.h"
#include "lidar.h"

static bool lastButtonState = HIGH;
static unsigned long buttonDownTime = 0;
static bool longPressHandled = false;

void handleButton() {
  bool state = digitalRead(BUTTON_PIN);

  if (state == LOW && lastButtonState == HIGH) { buttonDownTime = millis(); longPressHandled = false; }

  if (state == LOW && !longPressHandled && millis() - buttonDownTime > 2000) {
    startupAnimation();
    longPressHandled = true;
  }

  if (state == HIGH && lastButtonState == LOW) {
    unsigned long held = millis() - buttonDownTime;
    if (held < 2000 && !longPressHandled) {
      if (mode == AUTO) { mode = MANUAL; ledManual = true; Serial.println("Mode: MANUAL"); }
      else { currentAngle += 15; if (currentAngle > 150) currentAngle = 30; setServoAngle(HEAD_CHANNEL, currentAngle); }
    }
  }
  lastButtonState = state;
}
