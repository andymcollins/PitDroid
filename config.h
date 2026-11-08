#pragma once
#include <Arduino.h>

extern const char* ssid;
extern const char* password;

#define SDA_PIN      21
#define SCL_PIN      22
#define LED_PIN       4
#define BUTTON_PIN    5

#define PCA9685_ADDR 0x40
#define SERVO_MIN    150
#define SERVO_MAX    600
#define HEAD_CHANNEL 0

#define TRACK_DISTANCE_MM 500
#define LED_COUNT          8

enum Mode { AUTO, MANUAL };
extern Mode mode;
extern int currentAngle;
extern int manualAngle;
extern int ledR, ledG, ledB;
extern bool ledManual;
