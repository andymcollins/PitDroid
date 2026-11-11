#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "web_page.h"
#include "web_auth.h"
#include "servo_control.h"
#include "radar.h"
#include "leds.h"
#include "calibration.h"
#include "radar_test.h"
#include "controller.h"

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

Mode mode = AUTO;
int panAngle = 90, tiltAngle = 90, armAngle = 0, hipAngle = 90;
int armLAngle = 0, armRAngle = 0;

Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

static bool     haveTarget   = false;
static uint32_t lastTargetMs = 0;
static uint32_t lastTrackMs  = 0;
static int8_t   sweepDir     = 1;

// ---------------------------------------------------------------------------
// Behaviour
// ---------------------------------------------------------------------------
static void idleSweep() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < SWEEP_INTERVAL_MS) return;
  lastMs = now;

  panAngle += sweepDir * SWEEP_STEP_DEG;
  if (panAngle >= SWEEP_MAX_DEG) { panAngle = SWEEP_MAX_DEG; sweepDir = -1; }
  else if (panAngle <= SWEEP_MIN_DEG) { panAngle = SWEEP_MIN_DEG; sweepDir = 1; }

  if (SERVO_PRESENT(CH_PAN)) setServoAngle(CH_PAN, panAngle);
  ledSetStatus(0, 0, 255);
}

static void autoTrack() {
  uint32_t now = millis();
  if (now - lastTrackMs < TRACK_INTERVAL_MS) return;
  lastTrackMs = now;

  RadarTarget t = radarRead();
  if (!t.valid && radarTestActive()) {
    t.valid = true;
    t.angleDeg = radarTestAngle();
    t.distanceMm = radarTestDist();
  }
  if (t.valid) {
    lastTargetMs = now;
    haveTarget = true;
  }

  if (!haveTarget || now - lastTargetMs > TRACK_TIMEOUT_MS) {
    haveTarget = false;
    idleSweep();
    return;
  }
  if (!t.valid) return;  // hold the last pose between radar frames

  int targetPan = constrain(90 + (int)(t.angleDeg * 0.9f), 0, 180);
  if (abs(targetPan - panAngle) > TRACK_DEADZONE_DEG) {
    panAngle += (targetPan > panAngle) ? 2 : -2;
    panAngle = constrain(panAngle, 0, 180);
    if (SERVO_PRESENT(CH_PAN)) setServoAngle(CH_PAN, panAngle);
  }

  int targetTilt = map(constrain(t.distanceMm, 200, 3000), 200, 3000, 120, 60);
  if (targetTilt != tiltAngle) {
    tiltAngle += (targetTilt > tiltAngle) ? 1 : -1;
    tiltAngle = constrain(tiltAngle, 0, 180);
    if (SERVO_PRESENT(CH_TILT)) setServoAngle(CH_TILT, tiltAngle);
  }

  armAngle = (t.distanceMm < ARM_RAISE_DIST_MM) ? 45 : 0;
  armLAngle = armAngle;
  armRAngle = armAngle;
  if (SERVO_PRESENT(CH_ARM_L)) setServoAngle(CH_ARM_L, armLAngle);
  if (SERVO_PRESENT(CH_ARM_R)) setServoAngle(CH_ARM_R, armRAngle);

  hipAngle = constrain(90 + (int)(t.angleDeg * HIP_FOLLOW_FACTOR), 60, 120);
  if (SERVO_PRESENT(CH_HIP)) setServoAngle(CH_HIP, hipAngle);

  ledSetStatus(0, 255, 0);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  calibrationBegin();

  if (SERVO_PRESENT(CH_PAN))  setServoAngle(CH_PAN, panAngle);
  if (SERVO_PRESENT(CH_TILT)) setServoAngle(CH_TILT, tiltAngle);
  if (SERVO_PRESENT(CH_ARM_L)) setServoAngle(CH_ARM_L, armLAngle);
  if (SERVO_PRESENT(CH_ARM_R)) setServoAngle(CH_ARM_R, armRAngle);
  if (SERVO_PRESENT(CH_HIP))  setServoAngle(CH_HIP, hipAngle);

  ledBegin();
  radarBegin();

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  webRoute("/", []() { server.send_P(200, "text/html", INDEX_HTML); });

  webRoute("/servo", []() {
    if (!server.hasArg("ch") || !server.hasArg("angle")) {
      server.send(400, "text/plain", "ch and angle required");
      return;
    }
    int ch = server.arg("ch").toInt();
    int a  = constrain(server.arg("angle").toInt(), 0, 180);
    if (ch < 0 || ch > 4 || !SERVO_PRESENT(ch)) {
      server.send(400, "text/plain", "Channel not connected");
      return;
    }
    if (ch == CH_PAN) panAngle = a;
    else if (ch == CH_TILT) tiltAngle = a;
    else if (ch == CH_ARM_L) armLAngle = a;
    else if (ch == CH_ARM_R) armRAngle = a;
    else if (ch == CH_HIP) hipAngle = a;
    armAngle = (armLAngle + armRAngle) / 2;
    setServoAngle(ch, a);
    mode = MANUAL;  // the user took over; Auto/Idle must be re-selected
    server.send(200, "text/plain", "OK");
  });

  webRoute("/servo/angles", []() {
    String j = "{\"pan\":" + String(panAngle) +
               ",\"tilt\":" + String(tiltAngle) +
               ",\"armL\":" + String(armLAngle) +
               ",\"armR\":" + String(armRAngle) +
               ",\"hip\":" + String(hipAngle) + "}";
    server.send(200, "application/json", j);
  });

  webRoute("/led", []() {
    if (!server.hasArg("r") || !server.hasArg("g") || !server.hasArg("b")) {
      server.send(400, "text/plain", "r, g and b required");
      return;
    }
    ledSetColor(constrain(server.arg("r").toInt(), 0, 255),
                constrain(server.arg("g").toInt(), 0, 255),
                constrain(server.arg("b").toInt(), 0, 255));
    server.send(200, "text/plain", "OK");
  });

  webRoute("/anim", []() {
    if (!server.hasArg("mode")) {
      server.send(400, "text/plain", "mode required");
      return;
    }
    if (server.hasArg("speed")) ledSetSpeed(server.arg("speed").toInt());
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
      ledSetColor(constrain(server.arg("r").toInt(), 0, 255),
                  constrain(server.arg("g").toInt(), 0, 255),
                  constrain(server.arg("b").toInt(), 0, 255));
    }
    ledSetMode(server.arg("mode").toInt());
    server.send(200, "text/plain", "OK");
  });

  webRoute("/led/auto", []() { ledAuto(); server.send(200, "text/plain", "OK"); });

  webRoute("/brightness", []() {
    if (!server.hasArg("value")) {
      server.send(400, "text/plain", "value required");
      return;
    }
    ledSetBrightness(constrain(server.arg("value").toInt(), 0, 255));
    server.send(200, "text/plain", "OK");
  });

  webRoute("/mode", []() {
    String m = server.arg("set");
    if (m == "auto") {
      mode = AUTO;
      haveTarget = false;
    } else if (m == "idle") {
      mode = IDLE;
    } else if (m == "manual") {
      mode = MANUAL;
    } else {
      server.send(400, "text/plain", "set must be auto, idle or manual");
      return;
    }
    server.send(200, "text/plain", "OK");
  });

  webRoute("/animate", []() {
    startupAnimationBegin();
    server.send(200, "text/plain", "OK");
  });

  webRoute("/servo/channels", []() {
    String j = "[";
    bool first = true;
    for (uint8_t ch = 0; ch < 5; ch++) {
      if (!SERVO_PRESENT(ch)) continue;
      if (!first) j += ",";
      j += String(ch);
      first = false;
    }
    j += "]";
    server.send(200, "application/json", j);
  });

  calibrationHandler();
  radarTestHandler();
  controllerHandler();

  server.begin();
  Serial.println("Web server started");
  startupAnimationBegin();
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
  server.handleClient();
  calibrationUpdate();
  startupAnimationUpdate();

  // The startup animation drives the LEDs itself, so skip the pattern engine
  // while it runs.
  if (!startupAnimationRunning()) ledUpdate();

  // The animation and the calibration sweep own the servos while they run.
  if (!startupAnimationRunning() && !calibrationSweeping()) {
    switch (mode) {
      case AUTO:   autoTrack(); break;
      case IDLE:   idleSweep(); break;
      case MANUAL: break;
    }
  }

  delay(2);
}
