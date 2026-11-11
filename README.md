# Pit Droid — ESP32 Radar Head Tracking

ESP32 WROOM-32 firmware for a Pit Droid: RD-03D mmWave radar (chest) drives
pan/tilt servos via PCA9685, with 3 chained NeoPixel rings in the eye
(24 + 16 + 12 = 52 LEDs).

## Hardware
- 2S LiPo (7.4V) + fuse + switch
- Buck A 5V: ESP32 VIN, RD-03D, LED rings
- Buck B 6V: PCA9685 V+ (servo rail)
- ESP32 WROOM-32
- PCA9685 (I2C 0x40) — SDA=21, SCL=22
- RD-03D mmWave — 5V power, TX→GPIO16, RX optional→GPIO17
- 5x MG996R-capable channels: PCA9685 CH0 (pan), CH1 (tilt), CH2 (Left Arm),
  CH3 (Right Arm), CH4 (Hip). All five are enabled for motion by default
  (`SERVO_PRESENT_MASK 0x1F` in `config.h`); clear a bit for any channel that
  is not physically fitted.
- 3x WS2812B LED rings chained: 24 + 16 + 12 = 52 LEDs, DIN=GPIO4

## Libraries
- Adafruit PWM Servo Driver Library
- Adafruit NeoPixel

## Setup
1. Edit WiFi credentials in `PitDroid.ino`.
2. Upload to ESP32 Dev Module.
3. Open Serial Monitor at 115200 for IP.
4. On iPhone, open `http://<ip>/` for control, `/control` for the landscape
   manual controller, `/cal` for calibration, `/radar` for the radar test.

## Servo Calibration

Navigate to `http://<esp32-ip>/cal` after powering the droid.

1. Select a channel: 0 Pan, 1 Tilt, 2 Left Arm, 3 Right Arm, 4 Hip.
   You can also tap the numbered marker on the Pit Droid photo.
   All five can be calibrated; only channels enabled in `SERVO_PRESENT_MASK`
   are used for radar motion.
2. Use -5/+5 buttons or the slider to find the mechanical limits.
3. Save as Min, Max, and Center for each channel.
4. Values persist in NVS (flash) across reboots.
5. Test Sweep verifies the full travel.

PWM is clamped to 100-700 for MG996R safety.

## Modes

- **Auto** — radar tracking. Holds the last pose for `TRACK_TIMEOUT_MS` after a
  target disappears, then falls back to the idle sweep.
- **Idle** — pan sweep between `SWEEP_MIN_DEG` and `SWEEP_MAX_DEG`, reversing at
  the limits.
- **Manual** — servos hold position. Moving a slider on the control page
  switches to Manual automatically; press Auto or Idle to resume.

The startup animation and the calibration sweep run as non-blocking state
machines, so the web UI and radar keep responding while they play.

## Manual controller

`/control` is a full-screen landscape game-style pad: a joystick for head
pan/tilt, vertical sliders for each arm, and a horizontal hip slider. Every
input posts to `/servo?ch=&angle=` and switches the droid to Manual. The page
shows a rotate-to-landscape overlay when the device is held portrait.

## Radar

`radar.cpp` parses the RD-03D multi-target frame (`AA FF 03 00`, three 8-byte
targets, tail `55 CC`) and reports the nearest target as distance plus angle
(`atan2(x, y)`). Empty slots (`0,0`) are ignored.

## Web authentication

`config.h` has `WEB_AUTH_USER` / `WEB_AUTH_PASS`. Both are empty by default,
which leaves the UI open on the LAN. Set them to a non-empty user and password
to require HTTP basic auth on every route; the browser prompts once.

## Local preview

`tools/preview/server.py` mocks every endpoint in memory and reads the HTML out
of `web_page.h`, `calibration.cpp`, `radar_test.cpp` and `controller.cpp` at
startup, so restart it after editing any of those files:

```bash
python3 tools/preview/server.py --port 8765
```

Then open <http://127.0.0.1:8765/> (control), `/control` (manual controller),
`/cal` (calibration), `/radar` (radar test) or `/preview` (LED ring simulator).

## Credits

The calibration page uses a photo of this build supplied by the project owner.
Pit Droid and Star Wars are trademarks of Lucasfilm Ltd. / Disney.
