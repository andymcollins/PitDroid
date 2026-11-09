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

Mode mode = AUTO;
int panAngle = 90, tiltAngle = 90, armAngle = 0, hipAngle = 90;

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

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
  setServoAngle(CH_PAN, panAngle);
  setServoAngle(CH_TILT, tiltAngle);
  setServoAngle(CH_ARM_L, armAngle);
  setServoAngle(CH_ARM_R, armAngle);
  setServoAngle(CH_HIP, hipAngle);

  pixels.begin();
  pixels.setBrightness(80);
  pixels.clear(); pixels.show();

  radarBegin();

  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/servo", []() {
    int ch = server.hasArg("ch") ? server.arg("ch").toInt() : CH_PAN;
    int a  = constrain(server.arg("angle").toInt(), 0, 180);
    if (ch == CH_PAN) panAngle = a;
    else if (ch == CH_TILT) tiltAngle = a;
    else if (ch == CH_ARM_L || ch == CH_ARM_R) armAngle = a;
    else if (ch == CH_HIP) hipAngle = a;
    setServoAngle(ch, a);
    server.send(200, "text/plain", "OK");
  });
  server.on("/led", []() {
    setColor(constrain(server.arg("r").toInt(),0,255),
             constrain(server.arg("g").toInt(),0,255),
             constrain(server.arg("b").toInt(),0,255));
    server.send(200, "text/plain", "OK");
  });
  server.on("/brightness", []() {
    pixels.setBrightness(constrain(server.arg("value").toInt(),0,255));
    pixels.show();
    server.send(200, "text/plain", "OK");
  });
  server.on("/mode", []() {
    String m = server.arg("set");
    if (m == "auto") mode = AUTO;
    if (m == "idle") mode = IDLE;
    server.send(200, "text/plain", "OK");
  });
  server.on("/animate", []() { startupAnimation(); server.send(200, "text/plain", "OK"); });

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  server.begin();
  startupAnimation();
}

void loop() {
  server.handleClient();

  RadarTarget t = radarRead();
  if (t.valid) {
    int targetPan = 90 + (int)(t.angleDeg * 0.9f);
    if (abs(targetPan - panAngle) > TRACK_DEADZONE_DEG) {
      panAngle += (targetPan > panAngle) ? 2 : -2;
      setServoAngle(CH_PAN, panAngle);
    }
    int targetTilt = map(constrain(t.distanceMm, 200, 3000), 200, 3000, 120, 60);
    tiltAngle += (targetTilt > tiltAngle) ? 1 : (targetTilt < tiltAngle ? -1 : 0);
    setServoAngle(CH_TILT, tiltAngle);

    int targetArm = (t.distanceMm < ARM_RAISE_DIST_MM) ? 45 : 0;
    armAngle = targetArm;
    setServoAngle(CH_ARM_L, armAngle);
    setServoAngle(CH_ARM_R, armAngle);

    int targetHip = 90 + (int)(t.angleDeg * HIP_FOLLOW_FACTOR);
    hipAngle = constrain(targetHip, 60, 120);
    setServoAngle(CH_HIP, hipAngle);

    setColor(0, 255, 0);
    mode = AUTO;
  } else {
    panAngle += 1;
    if (panAngle > 135) panAngle = 45;
    setServoAngle(CH_PAN, panAngle);
    setColor(0, 0, 255);
    mode = IDLE;
  }
  delay(30);
}
