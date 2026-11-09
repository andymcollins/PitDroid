#pragma once
#include <Arduino.h>

// ---- Pins ----
#define SDA_PIN       21
#define SCL_PIN       22
#define RADAR_RX_PIN  16   // ESP32 RX <- radar TX
#define RADAR_TX_PIN  17   // optional, to radar RX
#define LED_PIN        4

// ---- PCA9685 ----
#define PCA9685_ADDR  0x40
#define SERVO_MIN     150
#define SERVO_MAX     600
#define CH_PAN        0
#define CH_TILT       1
#define CH_ARM_L      2
#define CH_ARM_R      3
#define CH_HIP        4

// All five PCA9685 channels stay defined, but only the bits set here are
// treated as physically connected for motion. The UI and motion code skip
// the rest. bit0 = Pan, bit1 = Tilt, bit2 = Arm L, bit3 = Arm R, bit4 = Hip.
#define SERVO_PRESENT_MASK 0x03   // 0x03 = Pan + Tilt only
#define SERVO_PRESENT(ch)  (((SERVO_PRESENT_MASK) >> (ch)) & 0x01)

// Channels shown on the calibration page. Default is all five so the arm and
// hip servos can be calibrated before they are enabled for motion.
#define SERVO_CAL_MASK 0x1F
#define SERVO_CALIBRATABLE(ch) (((SERVO_CAL_MASK) >> (ch)) & 0x01)

// ---- LED rings (24 + 16 + 12 = 52 addressable LEDs, chained) ----
#define RING_COUNT      3
#define RING0_LEDS     24   // outer ring
#define RING1_LEDS     16   // middle ring
#define RING2_LEDS     12   // inner ring
#define LED_COUNT      (RING0_LEDS + RING1_LEDS + RING2_LEDS)

// ---- Behaviour ----
#define TRACK_DEADZONE_DEG   5
#define TRACK_TIMEOUT_MS   2000
#define ARM_RAISE_DIST_MM   800
#define HIP_FOLLOW_FACTOR   0.3f

enum Mode { AUTO, IDLE };
extern Mode mode;
extern int panAngle, tiltAngle, armAngle, hipAngle;
