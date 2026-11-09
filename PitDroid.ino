#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "web_page.h"
#include "servo_control.h"
#include "radar.h"
#include "leds.h"
#include "calibration.h"

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

Mode mode = AUTO;
int panAngle = 90, tiltAngle = 90, armAngle = 0, hipAngle = 90;

Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

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
  if (SERVO_PRESENT(CH_ARM_L)) setServoAngle(CH_ARM_L, armAngle);
  if (SERVO_PRESENT(CH_ARM_R)) setServoAngle(CH_ARM_R, armAngle);
  if (SERVO_PRESENT(CH_HIP))  setServoAngle(CH_HIP, hipAngle);

  ledBegin();

  radarBegin();

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/servo", []() {
    int ch = server.hasArg("ch") ? server.arg("ch").toInt() : CH_PAN;
    int a  = constrain(server.arg("angle").toInt(), 0, 180);
    if (ch < 0 || ch > 4 || !SERVO_PRESENT(ch)) {
      server.send(400, "text/plain", "Channel not connected");
      return;
    }
    if (ch == CH_PAN) panAngle = a;
    else if (ch == CH_TILT) tiltAngle = a;
    else if (ch == CH_ARM_L || ch == CH_ARM_R) armAngle = a;
    else if (ch == CH_HIP) hipAngle = a;
    setServoAngle(ch, a);
    server.send(200, "text/plain", "OK");
  });
  server.on("/led", []() {
    ledSetColor(constrain(server.arg("r").toInt(),0,255),
                constrain(server.arg("g").toInt(),0,255),
                constrain(server.arg("b").toInt(),0,255));
    server.send(200, "text/plain", "OK");
  });
  server.on("/anim", []() {
    if (server.hasArg("speed")) ledSetSpeed(server.arg("speed").toInt());
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
      ledSetColor(constrain(server.arg("r").toInt(),0,255),
                  constrain(server.arg("g").toInt(),0,255),
                  constrain(server.arg("b").toInt(),0,255));
    }
    ledSetMode(server.arg("mode").toInt());
    server.send(200, "text/plain", "OK");
  });
  server.on("/led/auto", []() { ledAuto(); server.send(200, "text/plain", "OK"); });
  server.on("/brightness", []() {
    ledSetBrightness(constrain(server.arg("value").toInt(),0,255));
    server.send(200, "text/plain", "OK");
  });
  server.on("/mode", []() {
    String m = server.arg("set");
    if (m == "auto") mode = AUTO;
    if (m == "idle") mode = IDLE;
    server.send(200, "text/plain", "OK");
  });
  server.on("/animate", []() { startupAnimation(); server.send(200, "text/plain", "OK"); });

  server.on("/servo/channels", []() {
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

  server.begin();
  Serial.println("Web server started");
  startupAnimation();
}

void loop() {
  server.handleClient();
  ledUpdate();

  RadarTarget t = radarRead();
  if (t.valid) {
    int targetPan = 90 + (int)(t.angleDeg * 0.9f);
    if (abs(targetPan - panAngle) > TRACK_DEADZONE_DEG) {
      panAngle += (targetPan > panAngle) ? 2 : -2;
      if (SERVO_PRESENT(CH_PAN)) setServoAngle(CH_PAN, panAngle);
    }
    int targetTilt = map(constrain(t.distanceMm, 200, 3000), 200, 3000, 120, 60);
    tiltAngle += (targetTilt > tiltAngle) ? 1 : (targetTilt < tiltAngle ? -1 : 0);
    if (SERVO_PRESENT(CH_TILT)) setServoAngle(CH_TILT, tiltAngle);

    int targetArm = (t.distanceMm < ARM_RAISE_DIST_MM) ? 45 : 0;
    armAngle = targetArm;
    if (SERVO_PRESENT(CH_ARM_L)) setServoAngle(CH_ARM_L, armAngle);
    if (SERVO_PRESENT(CH_ARM_R)) setServoAngle(CH_ARM_R, armAngle);

    int targetHip = 90 + (int)(t.angleDeg * HIP_FOLLOW_FACTOR);
    hipAngle = constrain(targetHip, 60, 120);
    if (SERVO_PRESENT(CH_HIP)) setServoAngle(CH_HIP, hipAngle);

    ledSetStatus(0, 255, 0);
    mode = AUTO;
  } else {
    panAngle += 1;
    if (panAngle > 135) panAngle = 45;
    if (SERVO_PRESENT(CH_PAN)) setServoAngle(CH_PAN, panAngle);
    ledSetStatus(0, 0, 255);
    mode = IDLE;
  }
  delay(30);
}
