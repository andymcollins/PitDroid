# Pit Droid — ESP32 Web Control

ESP32 WROOM-32 firmware for a Pit Droid animatronic head: LiDAR tracking,
PCA9685 servo control, NeoPixel LEDs, and a browser-based control panel.

## Hardware
- ESP32 WROOM-32
- PCA9685 (I2C 0x40) — SDA=21, SCL=22, VCC=3.3V, V+=5V
- VL53L0X LiDAR (I2C 0x29) — SDA=21, SCL=22
- NeoPixel DIN=GPIO4
- Pushbutton GPIO5 to GND
- Head servo on PCA9685 channel 0

## Libraries
- Adafruit PWM Servo Driver Library
- Adafruit VL53L0X
- Adafruit NeoPixel

## Setup
1. Edit WiFi credentials in `PitDroid.ino`.
2. Upload to ESP32 Dev Module.
3. Open Serial Monitor at 115200 baud for the IP address.
4. On your iPhone, open that IP in Safari (same 2.4 GHz WiFi).
