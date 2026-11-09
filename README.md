# Pit Droid — ESP32 Radar Head Tracking

ESP32 WROOM-32 firmware for a Pit Droid: RD-03D mmWave radar (chest) drives
pan/tilt/arm/hip servos via PCA9685, with 3 chained NeoPixel rings in the eye.

## Hardware
- 2S LiPo (7.4V) + fuse + switch
- Buck A 5V: ESP32 VIN, RD-03D, LED rings
- Buck B 6V: PCA9685 V+ (servo rail)
- ESP32 WROOM-32
- PCA9685 (I2C 0x40) — SDA=21, SCL=22
- RD-03D mmWave — 5V power, TX→GPIO16, RX optional→GPIO17
- 5x MG996R on PCA9685 CH0–CH4
- 3x WS2812 LED rings chained, DIN=GPIO4

## Libraries
- Adafruit PWM Servo Driver Library
- Adafruit NeoPixel

## Setup
1. Edit WiFi credentials in `PitDroid.ino`.
2. Upload to ESP32 Dev Module.
3. Open Serial Monitor at 115200 for IP.
4. On iPhone, open `http://<ip>/` for control, `/cal` for calibration.

## Servo Calibration

Navigate to `http://<esp32-ip>/cal` after powering the droid.

1. Select a channel (0–4).
2. Use -5/+5 buttons or the slider to find the mechanical limits.
3. Save as Min, Max, and Center for each channel.
4. Values persist in NVS (flash) across reboots.
5. Test Sweep verifies the full travel.

PWM is clamped to 100-700 for MG996R safety.
