#include <Arduino.h>
#include <WebServer.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "radar_test.h"
#include "servo_control.h"
#include "web_auth.h"

extern WebServer server;
extern Mode mode;

static volatile int  simAngle = 0;
static volatile int  simDist  = 1500;
static volatile bool simActive = false;

extern int panAngle, tiltAngle;

const char RADAR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pit Droid // Radar Test</title>
<style>
:root{--cy:#22e0ff;--mg:#ff2d95;--lm:#a8ff3e;--ink:#dff6ff;--dim:#6fa8c7}
*{box-sizing:border-box}
html{min-height:100%}
body{margin:0 auto;padding:16px 16px 34px;max-width:520px;min-height:100vh;
  background:#04060c;color:var(--ink);
  font-family:ui-monospace,SFMono-Regular,Menlo,monospace;
  background-image:radial-gradient(1100px 520px at 50% -15%,#0e2338 0%,#04060c 62%);
  background-attachment:fixed;-webkit-tap-highlight-color:transparent}
body::before{content:"";position:fixed;inset:0;pointer-events:none;z-index:99;
  background:repeating-linear-gradient(0deg,rgba(180,240,255,.03) 0 1px,transparent 1px 3px)}
.hdr{display:flex;justify-content:space-between;align-items:center;
  padding-bottom:12px;margin-bottom:16px;font-size:10px;letter-spacing:.24em;
  color:var(--dim);border-bottom:1px solid rgba(34,224,255,.22)}
.hdr b{color:var(--cy);font-weight:600;text-shadow:0 0 10px rgba(34,224,255,.6)}
.card{position:relative;border:1px solid rgba(34,224,255,.28);border-radius:18px;
  background:linear-gradient(180deg,rgba(12,28,46,.92),rgba(4,8,14,.94));
  box-shadow:inset 0 0 40px rgba(34,224,255,.06),0 0 28px rgba(0,0,0,.55);
  padding:14px;margin-bottom:14px}
.card::before,.card::after{content:"";position:absolute;width:14px;height:14px;
  border:2px solid var(--cy);opacity:.5}
.card::before{top:7px;left:7px;border-right:0;border-bottom:0;border-radius:6px 0 0 0}
.card::after{bottom:7px;right:7px;border-left:0;border-top:0;border-radius:0 0 6px 0}
canvas{width:100%;height:auto;display:block;border-radius:12px;
  border:1px solid rgba(34,224,255,.25);background:#02060b}
.kv{display:flex;justify-content:space-between;font-size:11px;letter-spacing:.14em;
  text-transform:uppercase;color:var(--dim);margin-top:10px}
.kv span:last-child{color:var(--cy)}
.row{display:flex;gap:10px;margin-bottom:10px}
.row:last-child{margin-bottom:0}
.row button{flex:1;padding:13px;font-size:11px;font-family:inherit;letter-spacing:.14em;
  text-transform:uppercase;color:var(--ink);cursor:pointer;
  border:1px solid rgba(34,224,255,.55);border-radius:10px;
  background:linear-gradient(180deg,rgba(34,224,255,.16),rgba(34,224,255,.04));
  box-shadow:0 0 14px rgba(34,224,255,.14),inset 0 0 12px rgba(34,224,255,.06);
  transition:background .15s,box-shadow .15s,transform .06s}
.row button:hover{background:rgba(34,224,255,.26);box-shadow:0 0 22px rgba(34,224,255,.4)}
.row button:active{transform:translateY(1px)}
.row button.alt{border-color:rgba(111,168,199,.4);color:var(--dim);
  background:rgba(111,168,199,.08);box-shadow:none}
.row button.on{border-color:rgba(168,255,62,.7);color:var(--lm);
  background:rgba(168,255,62,.14);box-shadow:0 0 18px rgba(168,255,62,.35)}
</style>
</head>
<body>
<div class="hdr"><span><b>PIT DROID</b> // RADAR TEST</span><span>RD-03D</span></div>

<div class="card">
  <canvas id="c" width="500" height="380"></canvas>
  <div class="kv"><span>Target angle</span><span id="tAngle">0 deg</span></div>
  <div class="kv"><span>Target distance</span><span id="tDist">1500 mm</span></div>
</div>

<div class="card">
  <div class="kv" style="margin-top:0"><span>Pan servo</span><span id="sPan">90 deg</span></div>
  <div class="kv"><span>Tilt servo</span><span id="sTilt">90 deg</span></div>
</div>

<div class="row">
  <button id="btnAuto" class="alt" onclick="toggleAuto()">Auto Sweep</button>
  <button class="alt" onclick="clearTarget()">Clear Target</button>
</div>
<div class="row">
  <button onclick="location.href='/'">Back to Control</button>
</div>

<script>
const c = document.getElementById('c');
const ctx = c.getContext('2d');
const CX = c.width / 2;
const CY = c.height - 20;
const MAX_ANG = 60;
const MAX_DIST = 4000;
let target = { angle: 0, dist: 1500, active: false };
let servo = { pan: 90, tilt: 90 };
let auto = false;
let autoT = 0;

function draw() {
  ctx.clearRect(0, 0, c.width, c.height);
  ctx.strokeStyle = 'rgba(34,224,255,0.16)';
  ctx.lineWidth = 1;
  for (let r = 1; r <= 4; r++) {
    const rad = (r / 4) * (c.height - 40);
    ctx.beginPath();
    ctx.arc(CX, CY, rad, Math.PI, 2 * Math.PI);
    ctx.stroke();
  }
  ctx.strokeStyle = 'rgba(168,255,62,0.55)';
  ctx.beginPath();
  ctx.moveTo(CX, CY);
  const edge = (c.height - 40);
  const a1 = (-MAX_ANG) * Math.PI / 180;
  const a2 = ( MAX_ANG) * Math.PI / 180;
  ctx.lineTo(CX + Math.sin(a1) * edge, CY - Math.cos(a1) * edge);
  ctx.moveTo(CX, CY);
  ctx.lineTo(CX + Math.sin(a2) * edge, CY - Math.cos(a2) * edge);
  ctx.stroke();
  ctx.strokeStyle = 'rgba(111,168,199,0.35)';
  ctx.beginPath();
  ctx.moveTo(CX, CY);
  ctx.lineTo(CX, 20);
  ctx.stroke();

  // Live pan servo needle, driven by /radar/state.
  const panRad = (servo.pan - 90) * Math.PI / 180;
  ctx.save();
  ctx.shadowBlur = 14;
  ctx.shadowColor = '#ff2d95';
  ctx.strokeStyle = '#ff2d95';
  ctx.lineWidth = 4;
  ctx.beginPath();
  ctx.moveTo(CX, CY);
  ctx.lineTo(CX + Math.sin(panRad) * 150, CY - Math.cos(panRad) * 150);
  ctx.stroke();
  ctx.fillStyle = '#ff2d95';
  ctx.beginPath();
  ctx.arc(CX + Math.sin(panRad) * 150, CY - Math.cos(panRad) * 150, 6, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();
  ctx.lineWidth = 1;
  if (target.active) {
    const angRad = target.angle * Math.PI / 180;
    const distScale = (c.height - 40) * (target.dist / MAX_DIST);
    const x = CX + Math.sin(angRad) * distScale;
    const y = CY - Math.cos(angRad) * distScale;
    ctx.save();
    ctx.shadowBlur = 18;
    ctx.shadowColor = '#a8ff3e';
    ctx.fillStyle = '#a8ff3e';
    ctx.beginPath();
    ctx.arc(x, y, 10, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
    ctx.strokeStyle = '#eaffff';
    ctx.lineWidth = 2;
    ctx.stroke();
  }
  ctx.fillStyle = '#6fa8c7';
  ctx.font = '12px ui-monospace,monospace';
  ctx.fillText('-60', 10, CY - 4);
  ctx.fillText('+60', c.width - 40, CY - 4);
  ctx.fillText('4m', CX + 4, 32);
  ctx.fillText('0m', CX + 4, CY - 4);
}

function updateLabels() {
  document.getElementById('tAngle').innerText = target.angle.toFixed(0) + ' deg';
  document.getElementById('tDist').innerText  = target.dist + ' mm';
}

function sendTarget() {
  fetch('/radar/target?angle=' + target.angle.toFixed(1) +
        '&dist=' + target.dist +
        '&active=' + (target.active ? 1 : 0));
}

function poll() {
  fetch('/radar/state')
    .then(r => r.json())
    .then(d => {
      servo.pan = d.pan;
      servo.tilt = d.tilt;
      document.getElementById('sPan').innerText  = d.pan.toFixed(0) + ' deg';
      document.getElementById('sTilt').innerText = d.tilt.toFixed(0) + ' deg';
    });
}

function posToTarget(evt) {
  const rect = c.getBoundingClientRect();
  const px = (evt.touches ? evt.touches[0].clientX : evt.clientX) - rect.left;
  const py = (evt.touches ? evt.touches[0].clientY : evt.clientY) - rect.top;
  const x = px * (c.width / rect.width) - CX;
  const y = CY - py * (c.width / rect.width);
  let ang = Math.atan2(x, y) * 180 / Math.PI;
  ang = Math.max(-MAX_ANG, Math.min(MAX_ANG, ang));
  let dist = Math.sqrt(x*x + y*y) / (c.height - 40) * MAX_DIST;
  dist = Math.max(200, Math.min(MAX_DIST, dist));
  target.angle = ang;
  target.dist  = Math.round(dist);
  target.active = true;
  updateLabels();
  draw();
  sendTarget();
}

c.addEventListener('mousedown', posToTarget);
c.addEventListener('mousemove', e => { if (e.buttons === 1) posToTarget(e); });
c.addEventListener('touchstart', e => { e.preventDefault(); posToTarget(e); }, {passive:false});
c.addEventListener('touchmove',  e => { e.preventDefault(); posToTarget(e); }, {passive:false});

function toggleAuto() {
  auto = !auto;
  document.getElementById('btnAuto').classList.toggle('on', auto);
  document.getElementById('btnAuto').innerText = auto ? 'Stop Sweep' : 'Auto Sweep';
}

function clearTarget() {
  target.active = false;
  draw();
  sendTarget();
}

setInterval(() => {
  if (auto) {
    autoT += 0.05;
    target.angle = Math.sin(autoT) * MAX_ANG;
    target.dist  = 1500 + Math.sin(autoT * 2) * 800;
    target.active = true;
    updateLabels();
    sendTarget();
  }
  draw();
  poll();
}, 60);

draw();
updateLabels();
</script>
</body>
</html>
)rawliteral";

void radarTestHandler() {
  webRoute("/radar", []() { server.send_P(200, "text/html", RADAR_HTML); });

  webRoute("/radar/target", []() {
    simAngle  = constrain(server.arg("angle").toFloat(), -60, 60);
    simDist   = constrain(server.arg("dist").toInt(), 100, 4000);
    simActive = server.arg("active").toInt() == 1;
    if (simActive) mode = AUTO;  // injecting a target only matters while tracking
    server.send(200, "text/plain", "OK");
  });

  webRoute("/radar/state", []() {
    String j = "{\"pan\":" + String(panAngle) +
               ",\"tilt\":" + String(tiltAngle) + "}";
    server.send(200, "application/json", j);
  });
}

bool radarTestActive() { return simActive; }
int  radarTestAngle()  { return (int)simAngle; }
int  radarTestDist()   { return simDist; }
