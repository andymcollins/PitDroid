#pragma once
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pit Droid Control</title>
<style>
  body { font-family:-apple-system,sans-serif; background:#111; color:#eee;
         padding:20px; max-width:500px; margin:auto; }
  h2 { text-align:center; }
  .card { background:#222; border-radius:12px; padding:16px; margin-bottom:16px; }
  label { display:block; margin-bottom:6px; font-weight:600; }
  input[type=range] { width:100%; }
  input[type=color] { width:100%; height:50px; border:none; background:none; }
  select { width:100%; padding:10px; font-size:16px; border-radius:8px;
           border:1px solid #444; background:#111; color:#eee; }
  button { width:100%; padding:14px; font-size:16px; border:none;
           border-radius:10px; background:#0a84ff; color:#fff; margin-top:8px; }
  .val { float:right; color:#0a84ff; }
</style>
</head>
<body>
<h2>Pit Droid Control</h2>

<div class="card">
  <div id="servoSliders">
    <label>Servo</label>
    <div style="color:#888;font-size:13px">Loading...</div>
  </div>
</div>

<div class="card">
  <label>LED Color (solid)</label>
  <input type="color" id="color" value="#0000ff" onchange="sendColor(this.value)">
  <label style="margin-top:12px">Brightness <span class="val" id="briVal">80</span></label>
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
  <label style="margin-top:12px">Speed <span class="val" id="spdVal">50</span></label>
  <input type="range" min="1" max="100" value="50" id="spd"
         oninput="document.getElementById('spdVal').innerText=this.value">
  <button onclick="applyAnim()">Apply Animation</button>
  <button onclick="fetch('/led/auto')">Auto Status Colours</button>
</div>

<div class="card">
  <label>Mode</label>
  <button onclick="fetch('/mode?set=auto')">Auto (Radar Track)</button>
  <button onclick="fetch('/mode?set=idle')">Idle</button>
</div>

<div class="card">
  <button onclick="fetch('/animate')">Replay Startup Animation</button>
  <button onclick="location.href='/cal'">Servo Calibration</button>
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
    list = await (await fetch('/cal/channels')).json();
  } catch (e) {}
  box.innerHTML = '';
  list.forEach((ch, i) => {
    const label = document.createElement('label');
    label.innerHTML = (SERVO_NAMES[ch] || ('CH ' + ch)) +
                      ' <span class="val" id="v' + ch + '">90</span>';
    if (i > 0) label.style.marginTop = '14px';
    const input = document.createElement('input');
    input.type = 'range';
    input.min = 0; input.max = 180; input.value = 90;
    input.addEventListener('input', () => {
      document.getElementById('v' + ch).innerText = input.value;
    });
    input.addEventListener('change', () => {
      fetch('/servo?ch=' + ch + '&angle=' + input.value);
    });
    box.appendChild(label);
    box.appendChild(input);
  });
}
buildServoSliders();

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
