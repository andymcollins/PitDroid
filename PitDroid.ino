#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "web_page.h"
#include "servo_control.h"
#include "lidar.h"
#include "leds.h"
#include "button.h"

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

Mode mode = AUTO;
int currentAngle = 90;
int manualAngle  = 90;
int ledR = 0, ledG = 0, ledB = 255;
bool ledManual = false;

Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
Adafruit_VL53L0X lox;
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  setServoAngle(HEAD_CHANNEL, currentAngle);

  if (!lox.begin()) { Serial.println("VL53L0X not found!"); while (1); }

  pixels.begin();
  pixels.setBrightness(80);
  pixels.clear();
  pixels.show();

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });

  server.on("/servo", []() {
    if (!server.hasArg("angle")) { server.send(400, "text/plain", "missing angle"); return; }
    manualAngle = constrain(server.arg("angle").toInt(), 0, 180);
    if (mode == MANUAL) { currentAngle = manualAngle; setServoAngle(HEAD_CHANNEL, currentAngle); }
    server.send(200, "text/plain", "OK");
  });

  server.on("/led", []() {
    ledR = constrain(server.arg("r").toInt(), 0, 255);
    ledG = constrain(server.arg("g").toInt(), 0, 255);
    ledB = constrain(server.arg("b").toInt(), 0, 255);
    ledManual = true;
    setColor(ledR, ledG, ledB);
    server.send(200, "text/plain", "OK");
  });

  server.on("/brightness", []() {
    if (!server.hasArg("value")) { server.send(400, "text/plain", "missing value"); return; }
    pixels.setBrightness(constrain(server.arg("value").toInt(), 0, 255));
    pixels.show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/mode", []() {
    String m = server.arg("set");
    if (m == "auto")   { mode = AUTO;   ledManual = false; Serial.println("Mode: AUTO"); }
    if (m == "manual") { mode = MANUAL; ledManual = true;  Serial.println("Mode: MANUAL"); }
    server.send(200, "text/plain", "OK");
  });

  server.on("/animate", []() { startupAnimation(); server.send(200, "text/plain", "OK"); });

  server.begin();
  Serial.println("Web server started");
  startupAnimation();
}

void loop() {
  server.handleClient();
  handleButton();

  if (mode == AUTO && !ledManual)      runAutoMode();
  else if (mode == MANUAL)             setColor(ledR, ledG, ledB);

  delay(20);
}
