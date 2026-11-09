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

// ---- LED rings ----
#define LED_COUNT     36   // 3 rings x 12 LEDs

// ---- Behaviour ----
#define TRACK_DEADZONE_DEG   5
#define TRACK_TIMEOUT_MS   2000
#define ARM_RAISE_DIST_MM   800
#define HIP_FOLLOW_FACTOR   0.3f

enum Mode { AUTO, IDLE };
extern Mode mode;
extern int panAngle, tiltAngle, armAngle, hipAngle;
