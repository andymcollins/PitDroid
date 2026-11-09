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
  /* servo location diagram */
  .droid { width:100%; max-width:250px; margin:0 auto; }
  .droid svg { width:100%; height:auto; display:block; }
  .droid .part { fill:#1a1f27; stroke:#4a5563; stroke-width:1.6; stroke-linejoin:round; }
  .droid .part.dark { fill:#0e1218; }
  .droid .limb { fill:none; stroke:#4a5563; stroke-width:18;
                 stroke-linecap:round; stroke-linejoin:round; }
  .droid .eye { fill:#05080c; stroke:#4a5563; stroke-width:1.6; }
  .droid .glow { fill:#0a84ff; opacity:.55; }
  .droid .side { fill:#7d8796; font:9px -apple-system,sans-serif; letter-spacing:.08em; }
  .droid .srv { cursor:pointer; }
  .droid .srv rect { fill:#10243d; stroke:#0a84ff; stroke-width:1.6; }
  .droid .srv text { fill:#7fb0ff; font:11px ui-monospace,monospace;
                     text-anchor:middle; dominant-baseline:central; pointer-events:none; }
  .droid .srv.off { opacity:.22; cursor:default; }
  .droid .srv.active rect { fill:#0a84ff; stroke:#0a84ff; }
  .droid .srv.active text { fill:#08111c; }
  .droid .srv.hl rect, .droid .srv:hover rect { fill:#0a84ff; stroke:#0a84ff; }
  .droid .srv.hl text, .droid .srv:hover text { fill:#08111c; }
</style>
</head>
<body>
<h2>Servo Calibration</h2>

<div class="card">
  <label>Servo Locations</label>
  <div class="droid" id="droid"></div>
  <div class="small">Tap a marker to select that channel. Front view; labels follow the droid's own left/right.</div>
</div>

<div class="card">
  <label>Channel</label>
  <select id="ch" onchange="loadCh()"></select>
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
let availableChannels = [0, 1, 2, 3, 4];
const CHANNEL_NAMES = {0:'Pan', 1:'Tilt', 2:'Arm L', 3:'Arm R', 4:'Hip'};
// Marker positions in the SVG (front view). CH2/CH3 are the droid's own
// left/right, so they appear on the opposite side to the viewer.
const SERVO_POS = {
  0: {x:   0, y: 176},   // Pan  - neck base
  1: {x:   0, y: 140},   // Tilt - head/neck joint
  2: {x:  64, y: 206},   // Arm L - droid's left shoulder
  3: {x: -64, y: 206},   // Arm R - droid's right shoulder
  4: {x:   0, y: 318}    // Hip  - waist
};

function renderDroid() {
  const p = [];
  // antenna + head
  p.push('<line class="part" x1="0" y1="46" x2="0" y2="16"/>');
  p.push('<circle class="part" cx="0" cy="12" r="5"/>');
  p.push('<rect class="part" x="-46" y="46" width="92" height="88" rx="20"/>');
  p.push('<circle class="eye" cx="-20" cy="88" r="13"/>');
  p.push('<circle class="eye" cx="20" cy="88" r="13"/>');
  p.push('<circle class="glow" cx="-20" cy="88" r="5"/>');
  p.push('<circle class="glow" cx="20" cy="88" r="5"/>');
  p.push('<rect class="part dark" x="-18" y="114" width="36" height="9" rx="4"/>');
  // neck + torso + chest panel
  p.push('<rect class="part dark" x="-12" y="134" width="24" height="48" rx="6"/>');
  p.push('<rect class="part" x="-58" y="182" width="116" height="122" rx="18"/>');
  p.push('<rect class="part dark" x="-30" y="208" width="60" height="46" rx="8"/>');
  p.push('<line class="part" x1="-22" y1="222" x2="22" y2="222"/>');
  p.push('<line class="part" x1="-22" y1="236" x2="22" y2="236"/>');
  // arms + hands
  p.push('<path class="limb" d="M-58,200 L-84,258 L-78,306"/>');
  p.push('<path class="limb" d="M58,200 L84,258 L78,306"/>');
  p.push('<circle class="part dark" cx="-78" cy="310" r="10"/>');
  p.push('<circle class="part dark" cx="78" cy="310" r="10"/>');
  // waist + legs + feet
  p.push('<rect class="part dark" x="-44" y="302" width="88" height="30" rx="8"/>');
  p.push('<path class="limb" d="M-22,330 L-26,456"/>');
  p.push('<path class="limb" d="M22,330 L26,456"/>');
  p.push('<polygon class="part" points="-42,456 -12,456 -8,490 -48,490"/>');
  p.push('<polygon class="part" points="12,456 42,456 48,490 8,490"/>');
  p.push('<text class="side" x="-76" y="524" text-anchor="middle">DROID RIGHT</text>');
  p.push('<text class="side" x="76" y="524" text-anchor="middle">DROID LEFT</text>');
  // clickable servo tags
  for (const ch of [0, 1, 2, 3, 4]) {
    const s = SERVO_POS[ch];
    const on = availableChannels.includes(ch);
    p.push('<g class="srv' + (on ? '' : ' off') +
           (ch === currentCh ? ' active' : '') + '" data-ch="' + ch + '">' +
           '<title>CH' + ch + ' - ' + (CHANNEL_NAMES[ch] || ch) + '</title>' +
           '<rect x="' + (s.x - 17) + '" y="' + (s.y - 11) + '" width="34" height="22" rx="5"/>' +
           '<text x="' + s.x + '" y="' + (s.y + 1) + '">' + ch + '</text></g>');
  }
  const box = document.getElementById('droid');
  box.innerHTML = '<svg viewBox="-120 0 240 540" role="img" ' +
                  'aria-label="Pit Droid servo locations, front view">' + p.join('') + '</svg>';
  box.querySelectorAll('.srv:not(.off)').forEach(g => {
    const ch = parseInt(g.dataset.ch);
    g.addEventListener('click', () => selectChannel(ch));
    g.addEventListener('mouseenter', () => g.classList.add('hl'));
    g.addEventListener('mouseleave', () => g.classList.remove('hl'));
  });
}

function selectChannel(ch) {
  const sel = document.getElementById('ch');
  sel.value = String(ch);
  loadCh();
}

function updateDroidActive() {
  document.querySelectorAll('#droid .srv').forEach(g => {
    g.classList.toggle('active', parseInt(g.dataset.ch) === currentCh);
  });
}

async function loadChannels() {
  const sel = document.getElementById('ch');
  let list = [0];
  try {
    list = await (await fetch('/cal/channels')).json();
  } catch (e) {
    list = [0];
  }
  availableChannels = list;
  sel.innerHTML = '';
  list.forEach(ch => {
    const o = document.createElement('option');
    o.value = ch;
    o.textContent = ch + ' - ' + (CHANNEL_NAMES[ch] || ('CH ' + ch));
    sel.appendChild(o);
  });
  if (sel.options.length) currentCh = parseInt(sel.options[0].value);
  renderDroid();
  loadCh();
}
function loadCh() {
  currentCh = parseInt(document.getElementById('ch').value);
  updateDroidActive();
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
loadChannels();
</script>
</body>
</html>
)rawliteral";

static bool channelCalibratable(int ch) {
  return ch >= 0 && ch < 5 && SERVO_CALIBRATABLE((uint8_t)ch);
}

void calibrationHandler() {
  server.on("/cal", []() { server.send_P(200, "text/html", CAL_HTML); });

  server.on("/cal/channels", []() {
    String j = "[";
    bool first = true;
    for (uint8_t ch = 0; ch < 5; ch++) {
      if (!SERVO_CALIBRATABLE(ch)) continue;
      if (!first) j += ",";
      j += String(ch);
      first = false;
    }
    j += "]";
    server.send(200, "application/json", j);
  });

  server.on("/cal/values", []() {
    int ch = server.hasArg("ch") ? server.arg("ch").toInt() : 0;
    if (!channelCalibratable(ch)) { server.send(400, "text/plain", "Channel not available"); return; }
    ServoCal c = calibrationGet(ch);
    String j = "{\"min\":" + String(c.minPwm) +
               ",\"ctr\":" + String(c.centerPwm) +
               ",\"max\":" + String(c.maxPwm) + "}";
    server.send(200, "application/json", j);
  });

  server.on("/cal/pwm", []() {
    int ch = server.arg("ch").toInt();
    if (!channelCalibratable(ch)) { server.send(400, "text/plain", "Channel not available"); return; }
    int v = constrain(server.arg("v").toInt(), 100, 700);
    pwm.setPWM(ch, 0, v);
    server.send(200, "text/plain", "OK");
  });

  server.on("/cal/move", []() {
    int ch = server.arg("ch").toInt();
    if (!channelCalibratable(ch)) { server.send(400, "text/plain", "Channel not available"); return; }
    String to = server.arg("to");
    ServoCal c = calibrationGet(ch);
    int v = c.centerPwm;
    if (to == "min") v = c.minPwm;
    if (to == "max") v = c.maxPwm;
    pwm.setPWM(ch, 0, v);
    server.send(200, "text/plain", "OK");
  });

  server.on("/cal/save", []() {
    int ch = server.arg("ch").toInt();
    if (!channelCalibratable(ch)) { server.send(400, "text/plain", "Channel not available"); return; }
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
    int ch = server.arg("ch").toInt();
    if (!channelCalibratable(ch)) { server.send(400, "text/plain", "Channel not available"); return; }
    ServoCal c = calibrationGet(ch);
    for (int v = c.minPwm; v <= c.maxPwm; v += 10) { pwm.setPWM(ch, 0, v); delay(30); }
    for (int v = c.maxPwm; v >= c.minPwm; v -= 10) { pwm.setPWM(ch, 0, v); delay(30); }
    pwm.setPWM(ch, 0, c.centerPwm);
    server.send(200, "text/plain", "OK");
  });
}
