#pragma once
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pit Droid Control</title>
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
  padding:16px;margin-bottom:16px}
.card::before,.card::after{content:"";position:absolute;width:14px;height:14px;
  border:2px solid var(--cy);opacity:.5}
.card::before{top:7px;left:7px;border-right:0;border-bottom:0;border-radius:6px 0 0 0}
.card::after{bottom:7px;right:7px;border-left:0;border-top:0;border-radius:0 0 6px 0}
label{display:block;margin-bottom:8px;font-size:10px;letter-spacing:.22em;
  text-transform:uppercase;color:var(--dim)}
.val{float:right;color:var(--cy);letter-spacing:.06em}
select{width:100%;padding:11px 12px;font-size:13px;font-family:inherit;
  letter-spacing:.08em;border-radius:10px;border:1px solid rgba(34,224,255,.4);
  background:#08121c;color:var(--ink);outline:none}
select:focus{border-color:var(--cy);box-shadow:0 0 14px rgba(34,224,255,.3)}
button{width:100%;padding:13px;font-size:11px;font-family:inherit;letter-spacing:.16em;
  text-transform:uppercase;color:var(--ink);cursor:pointer;
  border:1px solid rgba(34,224,255,.55);border-radius:10px;
  background:linear-gradient(180deg,rgba(34,224,255,.16),rgba(34,224,255,.04));
  box-shadow:0 0 14px rgba(34,224,255,.14),inset 0 0 12px rgba(34,224,255,.06);
  transition:background .15s,box-shadow .15s,transform .06s}
button:hover{background:rgba(34,224,255,.26);box-shadow:0 0 22px rgba(34,224,255,.4)}
button:active{transform:translateY(1px)}
button+button{margin-top:8px}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:26px;
  background:transparent;margin:0}
input[type=range]::-webkit-slider-runnable-track{height:8px;border-radius:99px;
  background:linear-gradient(90deg,rgba(34,224,255,.28),rgba(255,45,149,.28));
  border:1px solid rgba(34,224,255,.38)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;
  margin-top:-6px;border-radius:5px;border:1px solid #eaffff;
  background:linear-gradient(180deg,#c9fbff,#22e0ff);
  box-shadow:0 0 12px rgba(34,224,255,.85)}
input[type=range]::-moz-range-track{height:8px;border-radius:99px;
  background:linear-gradient(90deg,rgba(34,224,255,.28),rgba(255,45,149,.28));
  border:1px solid rgba(34,224,255,.38)}
input[type=range]::-moz-range-thumb{width:18px;height:18px;border-radius:5px;
  border:1px solid #eaffff;background:#22e0ff;box-shadow:0 0 12px rgba(34,224,255,.85)}
input[type=color]{width:100%;height:50px;border:1px solid rgba(34,224,255,.4);
  border-radius:10px;background:#08121c;padding:4px;cursor:pointer}
.hint{color:var(--dim);font-size:11px;letter-spacing:.06em;margin-top:10px}
.slot+.slot{margin-top:16px}
</style>
</head>
<body>
<div class="hdr"><span><b>PIT DROID</b> // CONTROL</span><span>WEB</span></div>

<div class="card">
  <div id="servoSliders">
    <label>Servo</label>
    <div class="hint">Loading...</div>
  </div>
</div>

<div class="card">
  <label>LED Colour (Solid)</label>
  <input type="color" id="color" value="#0000ff" onchange="sendColor(this.value)">
  <label style="margin-top:16px">Brightness <span class="val" id="briVal">80</span></label>
  <input type="range" min="0" max="255" value="80" id="bri"
         oninput="document.getElementById('briVal').innerText=this.value"
         onchange="fetch('/brightness?value='+this.value)">
</div>

<div class="card">
  <label>LED Animation</label>
  <select id="anim">
    <option value="0">Solid</option>
    <option value="1">Rainbow</option>
    <option value="2">Chase</option>
    <option value="3">Comet</option>
    <option value="4">Breathe</option>
    <option value="5">Sparkle</option>
    <option value="6">Scanner (Knight Rider)</option>
    <option value="7">Theater Chase</option>
    <option value="8">Ring Cycle</option>
  </select>
  <label style="margin-top:16px">Speed <span class="val" id="spdVal">50</span></label>
  <input type="range" min="1" max="100" value="50" id="spd"
         oninput="document.getElementById('spdVal').innerText=this.value">
  <button onclick="applyAnim()" style="margin-top:14px">Apply Animation</button>
  <button onclick="fetch('/led/auto')">Auto Status Colours</button>
</div>

<div class="card">
  <label>Mode</label>
  <button onclick="setMode('auto')">Auto (Radar Track)</button>
  <button onclick="setMode('idle')">Idle (Sweep)</button>
  <button onclick="setMode('manual')">Manual (Hold)</button>
  <div class="hint">Moving a servo slider switches to Manual.</div>
</div>

<div class="card">
  <button onclick="location.href='/control'">Manual Controller (Landscape)</button>
  <button onclick="fetch('/animate')">Replay Startup Animation</button>
  <button onclick="location.href='/cal'">Servo Calibration</button>
  <button onclick="location.href='/radar'">Radar Test</button>
  <button id="previewBtn" style="display:none" onclick="location.href='/preview'">LED Ring Preview</button>
</div>

<script>
// The ring preview is a simulator served by tools/preview/server.py, so the
// link is only shown when this page is opened from the local preview server.
if (location.hostname === '127.0.0.1' || location.hostname === 'localhost') {
  document.getElementById('previewBtn').style.display = 'block';
}

const SERVO_NAMES = {0:'Pan', 1:'Tilt', 2:'Arm L', 3:'Arm R', 4:'Hip'};
async function buildServoSliders() {
  const box = document.getElementById('servoSliders');
  let list = [0];
  try {
    list = await (await fetch('/servo/channels')).json();
  } catch (e) {}
  box.innerHTML = '';
  list.forEach((ch, i) => {
    const slot = document.createElement('div');
    slot.className = 'slot';
    const label = document.createElement('label');
    label.innerHTML = (SERVO_NAMES[ch] || ('CH ' + ch)) +
                      ' <span class="val" id="v' + ch + '">90</span>';
    const input = document.createElement('input');
    input.type = 'range';
    input.min = 0; input.max = 180; input.value = 90;
    input.addEventListener('input', () => {
      document.getElementById('v' + ch).innerText = input.value;
    });
    input.addEventListener('change', () => {
      fetch('/servo?ch=' + ch + '&angle=' + input.value);
    });
    slot.appendChild(label);
    slot.appendChild(input);
    box.appendChild(slot);
  });
}
buildServoSliders();

function setMode(m) {
  fetch('/mode?set=' + m);
}
function sendColor(hex) {
  const r = parseInt(hex.substr(1,2),16);
  const g = parseInt(hex.substr(3,2),16);
  const b = parseInt(hex.substr(5,2),16);
  fetch('/led?r='+r+'&g='+g+'&b='+b);
}
function applyAnim() {
  const m = document.getElementById('anim').value;
  const s = document.getElementById('spd').value;
  const hex = document.getElementById('color').value;
  const r = parseInt(hex.substr(1,2),16);
  const g = parseInt(hex.substr(3,2),16);
  const b = parseInt(hex.substr(5,2),16);
  fetch('/anim?mode='+m+'&speed='+s+'&r='+r+'&g='+g+'&b='+b);
}
</script>
</body>
</html>
)rawliteral";
