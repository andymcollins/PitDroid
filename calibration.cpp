#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "calibration.h"

extern Adafruit_PWMServoDriver pwm;
extern WebServer server;

static Preferences prefs;
static ServoCal cal[16];

static const int DEF_MIN    = SERVO_MIN;
static const int DEF_MAX    = SERVO_MAX;
static const int DEF_CENTER = (SERVO_MIN + SERVO_MAX) / 2;

static String key(uint8_t ch, const char* suffix) {
  return "ch" + String(ch) + "_" + suffix;
}

void calibrationBegin() {
  prefs.begin("servocal", false);
  for (uint8_t ch = 0; ch < 16; ch++) {
    cal[ch].minPwm    = prefs.getInt(key(ch, "min").c_str(), DEF_MIN);
    cal[ch].maxPwm    = prefs.getInt(key(ch, "max").c_str(), DEF_MAX);
    cal[ch].centerPwm = prefs.getInt(key(ch, "ctr").c_str(), DEF_CENTER);
  }
}

ServoCal calibrationGet(uint8_t ch) {
  if (ch >= 16) ch = 0;
  return cal[ch];
}

void calibrationSave(uint8_t ch, int minPwm, int maxPwm, int centerPwm) {
  if (ch >= 16) return;
  cal[ch].minPwm    = constrain(minPwm,    100, 700);
  cal[ch].maxPwm    = constrain(maxPwm,    100, 700);
  cal[ch].centerPwm = constrain(centerPwm, 100, 700);
  if (cal[ch].minPwm > cal[ch].maxPwm) {
    int t = cal[ch].minPwm; cal[ch].minPwm = cal[ch].maxPwm; cal[ch].maxPwm = t;
  }
  prefs.putInt(key(ch, "min").c_str(), cal[ch].minPwm);
  prefs.putInt(key(ch, "max").c_str(), cal[ch].maxPwm);
  prefs.putInt(key(ch, "ctr").c_str(), cal[ch].centerPwm);
}

const char CAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pit Droid - Servo Calibration</title>
<style>
  body { font-family:-apple-system,sans-serif; background:#111; color:#eee;
         padding:16px; max-width:520px; margin:auto; }
  h2 { text-align:center; margin:4px 0 12px; }
  .card { background:#222; border-radius:12px; padding:14px; margin-bottom:14px; }
  label { display:block; margin-bottom:6px; font-weight:600; }
  select { width:100%; padding:10px; font-size:16px; border-radius:8px;
           border:1px solid #444; background:#111; color:#eee; }
  input[type=range] { width:100%; }
  .row { display:flex; gap:8px; margin-top:8px; }
  .row button { flex:1; padding:12px; font-size:16px; border:none;
         border-radius:10px; background:#0a84ff; color:#fff; }
  .row button.alt { background:#444; }
  .row button.save { background:#30a14e; }
  .row button.warn { background:#c93c37; }
  .val { float:right; color:#0a84ff; }
  .small { font-size:12px; color:#888; text-align:center; margin-top:8px; }
</style>
</head>
<body>
<h2>Servo Calibration</h2>

<div class="card">
  <label>Channel</label>
  <select id="ch" onchange="loadCh()">
    <option value="0">0 - Pan</option>
    <option value="1">1 - Tilt</option>
    <option value="2">2 - Arm L</option>
    <option value="3">3 - Arm R</option>
    <option value="4">4 - Hip</option>
  </select>
</div>

<div class="card">
  <label>PWM <span class="val" id="pwmVal">375</span></label>
  <input type="range" min="100" max="700" value="375" id="pwm">
  <div class="row">
    <button class="alt" onclick="nudge(-5)">-5</button>
    <button class="alt" onclick="nudge(-1)">-1</button>
    <button class="alt" onclick="nudge(1)">+1</button>
    <button class="alt" onclick="nudge(5)">+5</button>
  </div>
  <div class="row">
    <button class="warn" onclick="sendPwm(100)">100</button>
    <button class="warn" onclick="sendPwm(375)">Center</button>
    <button class="warn" onclick="sendPwm(700)">700</button>
  </div>
  <div class="row">
    <button class="alt" onclick="moveTo('min')">Go Min</button>
    <button class="alt" onclick="moveTo('ctr')">Go Center</button>
    <button class="alt" onclick="moveTo('max')">Go Max</button>
  </div>
  <div class="row">
    <button class="save" onclick="saveAs('min')">Save as Min</button>
    <button class="save" onclick="saveAs('ctr')">Save as Center</button>
    <button class="save" onclick="saveAs('max')">Save as Max</button>
  </div>
  <div class="row">
    <button onclick="sweep()">Test Sweep Min-Max</button>
  </div>
  <div class="small">PWM clamped to 100-700 for MG996R safety.</div>
</div>

<div class="card">
  <label>Saved values for this channel</label>
  <div id="saved" style="font-size:14px; color:#ccc;">-</div>
</div>

<div class="row">
  <button onclick="location.href='/'">Back to Control</button>
</div>

<script>
let currentCh = 0;
function loadCh() {
  currentCh = parseInt(document.getElementById('ch').value);
  fetch('/cal/values?ch=' + currentCh)
    .then(r => r.json())
    .then(d => {
      document.getElementById('saved').innerText =
        'min=' + d.min + '  center=' + d.ctr + '  max=' + d.max;
    });
}
function sendPwm(v) {
  v = Math.max(100, Math.min(700, v));
  document.getElementById('pwm').value = v;
  document.getElementById('pwmVal').innerText = v;
  fetch('/cal/pwm?ch=' + currentCh + '&v=' + v);
}
function nudge(d) {
  sendPwm(parseInt(document.getElementById('pwm').value) + d);
}
function moveTo(which) { fetch('/cal/move?ch=' + currentCh + '&to=' + which); }
function saveAs(which) {
  const v = parseInt(document.getElementById('pwm').value);
  fetch('/cal/save?ch=' + currentCh + '&which=' + which + '&v=' + v).then(loadCh);
}
function sweep() { fetch('/cal/sweep?ch=' + currentCh); }
document.getElementById('pwm').addEventListener('input', function() {
  document.getElementById('pwmVal').innerText = this.value;
  fetch('/cal/pwm?ch=' + currentCh + '&v=' + this.value);
});
loadCh();
</script>
</body>
</html>
)rawliteral";

void calibrationHandler() {
  server.on("/cal", []() { server.send_P(200, "text/html", CAL_HTML); });

  server.on("/cal/values", []() {
    uint8_t ch = server.hasArg("ch") ? server.arg("ch").toInt() : 0;
    ServoCal c = calibrationGet(ch);
    String j = "{\"min\":" + String(c.minPwm) +
               ",\"ctr\":" + String(c.centerPwm) +
               ",\"max\":" + String(c.maxPwm) + "}";
    server.send(200, "application/json", j);
  });

  server.on("/cal/pwm", []() {
    uint8_t ch = server.arg("ch").toInt();
    int v = constrain(server.arg("v").toInt(), 100, 700);
    pwm.setPWM(ch, 0, v);
    server.send(200, "text/plain", "OK");
  });

  server.on("/cal/move", []() {
    uint8_t ch = server.arg("ch").toInt();
    String to = server.arg("to");
    ServoCal c = calibrationGet(ch);
    int v = c.centerPwm;
    if (to == "min") v = c.minPwm;
    if (to == "max") v = c.maxPwm;
    pwm.setPWM(ch, 0, v);
    server.send(200, "text/plain", "OK");
  });

  server.on("/cal/save", []() {
    uint8_t ch = server.arg("ch").toInt();
    String which = server.arg("which");
    int v = constrain(server.arg("v").toInt(), 100, 700);
    ServoCal c = calibrationGet(ch);
    if (which == "min") c.minPwm = v;
    else if (which == "max") c.maxPwm = v;
    else if (which == "ctr") c.centerPwm = v;
    calibrationSave(ch, c.minPwm, c.maxPwm, c.centerPwm);
    server.send(200, "text/plain", "OK");
  });

  server.on("/cal/sweep", []() {
    uint8_t ch = server.arg("ch").toInt();
    ServoCal c = calibrationGet(ch);
    for (int v = c.minPwm; v <= c.maxPwm; v += 10) { pwm.setPWM(ch, 0, v); delay(30); }
    for (int v = c.maxPwm; v >= c.minPwm; v -= 10) { pwm.setPWM(ch, 0, v); delay(30); }
    pwm.setPWM(ch, 0, c.centerPwm);
    server.send(200, "text/plain", "OK");
  });
}
