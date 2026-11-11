#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "controller.h"
#include "web_auth.h"

extern WebServer server;

const char CONTROLLER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Pit Droid // Controller</title>
<style>
:root{--cy:#22e0ff;--mg:#ff2d95;--lm:#a8ff3e;--ink:#dff6ff;--dim:#6fa8c7}
*{box-sizing:border-box}
html,body{margin:0;height:100%;overflow:hidden;background:#04060c;color:var(--ink);
  font-family:ui-monospace,SFMono-Regular,Menlo,monospace;-webkit-user-select:none;user-select:none;
  -webkit-tap-highlight-color:transparent;touch-action:none}
body{display:flex;flex-direction:column;
  background:radial-gradient(1100px 520px at 50% -15%,#0e2338 0%,#04060c 62%)}
.scan{position:fixed;inset:0;pointer-events:none;z-index:40;
  background:repeating-linear-gradient(0deg,rgba(180,240,255,.035) 0 1px,transparent 1px 3px)}
header{display:flex;justify-content:space-between;align-items:center;
  padding:10px 18px;font-size:11px;letter-spacing:.22em;color:var(--dim);
  border-bottom:1px solid rgba(34,224,255,.22)}
header b{color:var(--cy);font-weight:600}
.badge{color:var(--lm)}
a.exit{color:var(--mg);text-decoration:none;letter-spacing:.18em;font-size:11px;
  border:1px solid rgba(255,45,149,.5);padding:6px 12px;border-radius:8px;
  box-shadow:0 0 14px rgba(255,45,149,.25)}
main{flex:1;display:flex;gap:16px;padding:16px;min-height:0}
.panel{position:relative;border:1px solid rgba(34,224,255,.28);border-radius:18px;
  background:linear-gradient(180deg,rgba(12,28,46,.92),rgba(4,8,14,.94));
  box-shadow:inset 0 0 40px rgba(34,224,255,.06),0 0 28px rgba(0,0,0,.55);
  padding:14px;display:flex;flex-direction:column;min-height:0}
.panel::before,.panel::after{content:"";position:absolute;width:16px;height:16px;border:2px solid var(--cy);opacity:.55}
.panel::before{top:8px;left:8px;border-right:0;border-bottom:0;border-radius:6px 0 0 0}
.panel::after{bottom:8px;right:8px;border-left:0;border-top:0;border-radius:0 0 6px 0}
.ptitle{font-size:10px;letter-spacing:.28em;color:var(--dim);margin:2px 0 10px;text-align:center}
.left{flex:0 0 42%}
.right{flex:1;gap:12px}
.stickwrap{flex:1;display:grid;place-items:center;min-height:0}
.stick{position:relative;width:min(52vh,300px);aspect-ratio:1;border-radius:50%;
  background:radial-gradient(circle at 50% 45%,#0d2236,#050a12 70%);
  border:1px solid rgba(34,224,255,.35);
  box-shadow:inset 0 0 60px rgba(34,224,255,.10),0 0 30px rgba(34,224,255,.12);
  touch-action:none;cursor:grab}
.stick::after{content:"";position:absolute;inset:12%;border-radius:50%;
  border:1px dashed rgba(34,224,255,.22)}
.ring{position:absolute;inset:22%;border-radius:50%;border:1px solid rgba(255,45,149,.25)}
.cross{position:absolute;inset:0}
.cross::before,.cross::after{content:"";position:absolute;background:rgba(34,224,255,.18)}
.cross::before{left:8%;right:8%;top:50%;height:1px}
.cross::after{top:8%;bottom:8%;left:50%;width:1px}
.knob{position:absolute;left:50%;top:50%;width:32%;aspect-ratio:1;border-radius:50%;
  transform:translate(-50%,-50%);
  background:radial-gradient(circle at 40% 35%,#bff6ff,#22e0ff 45%,#0a6a86 100%);
  border:1px solid #cff;box-shadow:0 0 24px rgba(34,224,255,.65),inset 0 -6px 14px rgba(0,0,0,.45);
  transition:transform .08s ease-out}
.read{display:flex;justify-content:space-around;font-size:12px;color:var(--dim);
  letter-spacing:.12em;padding-top:8px}
.read b{color:var(--cy);font-size:16px}
.sliders{flex:1;display:flex;justify-content:space-around;gap:10px;min-height:0}
.vslot{display:flex;flex-direction:column;align-items:center;gap:8px;flex:1;min-height:0}
.vlab{font-size:10px;letter-spacing:.2em;color:var(--dim)}
.vval{font-size:12px;color:var(--lm)}
input[type=range]{-webkit-appearance:none;appearance:none;background:transparent;margin:0}
input[type=range].v{-webkit-appearance:slider-vertical;appearance:slider-vertical;
  writing-mode:vertical-lr;direction:rtl;width:46px;flex:1;min-height:60px}
input[type=range]::-webkit-slider-runnable-track{
  background:linear-gradient(180deg,rgba(34,224,255,.18),rgba(255,45,149,.18));
  border:1px solid rgba(34,224,255,.4);border-radius:999px}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:38px;height:16px;
  border-radius:5px;background:linear-gradient(180deg,#c9fbff,#22e0ff);
  border:1px solid #eaffff;box-shadow:0 0 14px rgba(34,224,255,.8)}
input[type=range].h{width:100%;height:44px}
input[type=range].h::-webkit-slider-thumb{width:16px;height:38px}
.hip{display:flex;flex-direction:column;gap:6px}
#rotate{position:fixed;inset:0;z-index:60;display:none;place-items:center;background:#04060c;
  color:var(--cy);letter-spacing:.34em;font-size:13px;text-align:center;padding:30px}
@media (orientation:portrait){#rotate{display:grid}}
</style>
</head>
<body>
<div class="scan"></div>
<div id="rotate">ROTATE DEVICE<br>LANDSCAPE REQUIRED</div>

<header>
  <span><b>PIT DROID</b> // MANUAL CONTROL</span>
  <span class="badge" id="mode">MANUAL</span>
  <a class="exit" href="/">EXIT</a>
</header>

<main>
  <section class="panel left">
    <div class="ptitle">HEAD // PAN + TILT</div>
    <div class="stickwrap">
      <div class="stick" id="stick">
        <div class="ring"></div><div class="cross"></div>
        <div class="knob" id="knob"></div>
      </div>
    </div>
    <div class="read">
      <span>PAN <b id="panv">90</b></span>
      <span>TILT <b id="tiltv">90</b></span>
    </div>
  </section>

  <section class="panel right">
    <div class="ptitle">LIMBS</div>
    <div class="sliders">
      <div class="vslot">
        <span class="vlab">ARM L</span>
        <input class="v" id="armL" type="range" min="0" max="180" value="0">
        <span class="vval" id="armLv">0</span>
      </div>
      <div class="vslot">
        <span class="vlab">ARM R</span>
        <input class="v" id="armR" type="range" min="0" max="180" value="0">
        <span class="vval" id="armRv">0</span>
      </div>
    </div>
    <div class="hip">
      <span class="vlab" style="text-align:center">HIP</span>
      <input class="h" id="hip" type="range" min="0" max="180" value="90">
      <span class="vval" id="hipv" style="text-align:center">90</span>
    </div>
  </section>
</main>

<script>
var st = {pan:90, tilt:90, armL:0, armR:0, hip:90};
var last = {};
function send(ch, val){
  var now = Date.now();
  if (now - (last[ch] || 0) < 55) return;
  last[ch] = now;
  fetch('/servo?ch=' + ch + '&angle=' + Math.round(val)).catch(function(){});
}
function clamp(v){ return Math.max(0, Math.min(180, v)); }
function draw(){
  document.getElementById('panv').textContent  = Math.round(st.pan);
  document.getElementById('tiltv').textContent = Math.round(st.tilt);
  document.getElementById('armLv').textContent = Math.round(st.armL);
  document.getElementById('armRv').textContent = Math.round(st.armR);
  document.getElementById('hipv').textContent  = Math.round(st.hip);
}

var stick = document.getElementById('stick');
var knob  = document.getElementById('knob');
var dragging = false;

function moveStick(ev){
  if (!dragging) return;
  var r = stick.getBoundingClientRect();
  var cx = r.left + r.width / 2;
  var cy = r.top  + r.height / 2;
  var dx = (ev.clientX - cx) / (r.width * 0.34);
  var dy = (ev.clientY - cy) / (r.height * 0.34);
  var m = Math.sqrt(dx*dx + dy*dy);
  if (m > 1){ dx /= m; dy /= m; }
  knob.style.transform = 'translate(calc(-50% + ' + (dx * r.width * 0.22) +
                         'px), calc(-50% + ' + (dy * r.height * 0.22) + 'px))';
  st.pan  = clamp(90 + dx * 90);
  st.tilt = clamp(90 - dy * 90);
  send(0, st.pan);
  send(1, st.tilt);
  draw();
}
function endStick(){
  dragging = false;
  knob.style.transform = 'translate(-50%,-50%)';
}
stick.addEventListener('pointerdown', function(e){
  dragging = true;
  stick.setPointerCapture(e.pointerId);
  moveStick(e);
});
stick.addEventListener('pointermove', moveStick);
stick.addEventListener('pointerup', endStick);
stick.addEventListener('pointercancel', endStick);

function bind(id, ch, key){
  var el = document.getElementById(id);
  el.addEventListener('input', function(){
    st[key] = parseFloat(el.value);
    send(ch, st[key]);
    draw();
  });
}
bind('armL', 2, 'armL');
bind('armR', 3, 'armR');
bind('hip',  4, 'hip');

function sync(){
  fetch('/servo/angles')
    .then(function(r){ return r.json(); })
    .then(function(d){
      st.pan = d.pan; st.tilt = d.tilt;
      st.armL = d.armL; st.armR = d.armR; st.hip = d.hip;
      document.getElementById('armL').value = d.armL;
      document.getElementById('armR').value = d.armR;
      document.getElementById('hip').value  = d.hip;
      draw();
    })
    .catch(function(){});
}
sync();
draw();
</script>
</body>
</html>
)rawliteral";

void controllerHandler() {
  webRoute("/control", []() { server.send_P(200, "text/html", CONTROLLER_HTML); });
}
