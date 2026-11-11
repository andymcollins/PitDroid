#!/usr/bin/env python3
"""
Local preview server for the PitDroid web UI.

It reads INDEX_HTML from web_page.h and CAL_HTML from calibration.cpp at
startup, so the preview always matches the firmware. All ESP32 endpoints are
mocked in memory and every request is logged to the console.

Usage:
    python3 tools/preview/server.py [--port 8765]
"""

import argparse
import json
import re
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

ROOT = Path(__file__).resolve().parents[2]

CHANNELS = {0: "Pan", 1: "Tilt", 2: "Arm L", 3: "Arm R", 4: "Hip"}
state = {
    "mode": "auto",
    "brightness": 80,
    "led": (0, 0, 255),
    "anim": {"mode": 0, "speed": 50},
    "servo": {0: 90, 1: 90, 2: 0, 3: 0, 4: 90},
    "cal": {ch: {"min": 150, "ctr": 375, "max": 600} for ch in CHANNELS},
    "sim": {"angle": 0.0, "dist": 1500, "active": False},
}


def extract_raw(path: Path, name: str) -> str:
    text = path.read_text()
    # Match:  const char NAME[] PROGMEM = R"rawliteral( ... )rawliteral";
    m = re.search(
        r"const\s+char\s+" + re.escape(name) + r"\[\]\s*(?:PROGMEM\s*)?=\s*R\"rawliteral\((.*?)\)rawliteral\"",
        text,
        re.S,
    )
    if not m:
        raise SystemExit(f"Could not find {name} in {path}")
    return m.group(1)


INDEX_HTML = extract_raw(ROOT / "web_page.h", "INDEX_HTML")
CAL_HTML = extract_raw(ROOT / "calibration.cpp", "CAL_HTML")
RADAR_HTML = extract_raw(ROOT / "radar_test.cpp", "RADAR_HTML")
CONTROLLER_HTML = extract_raw(ROOT / "controller.cpp", "CONTROLLER_HTML")


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        sys.stderr.write("[preview] " + (fmt % args) + "\n")

    def _send(self, body, content_type="text/html; charset=utf-8", code=200):
        data = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _args(self):
        q = parse_qs(urlparse(self.path).query)
        return {k: v[0] for k, v in q.items()}

    def do_GET(self):
        path = urlparse(self.path).path
        a = self._args()

        if path == "/preview":
            return self._send((ROOT / "tools" / "preview" / "leds.html").read_text())

        if path == "/":
            return self._send(INDEX_HTML)
        if path == "/cal":
            return self._send(CAL_HTML)

        if path == "/radar":
            return self._send(RADAR_HTML)

        if path == "/control":
            return self._send(CONTROLLER_HTML)

        if path == "/radar/target":
            try:
                angle = float(a.get("angle", 0))
                dist = int(float(a.get("dist", 1500)))
            except ValueError:
                return self._send("Bad target", "text/plain", 400)
            state["sim"]["angle"] = max(-60.0, min(60.0, angle))
            state["sim"]["dist"] = max(100, min(4000, dist))
            state["sim"]["active"] = a.get("active", "0") == "1"
            if state["sim"]["active"]:
                state["mode"] = "auto"
            print(
                f"[radar/target] angle={state['sim']['angle']:.1f} "
                f"dist={state['sim']['dist']} active={state['sim']['active']}"
            )
            return self._send("OK", "text/plain")

        if path == "/radar/state":
            # Emulate the tracking loop so the test page shows servo motion.
            if state["mode"] == "auto" and state["sim"]["active"]:
                target_pan = 90 + state["sim"]["angle"] * 0.9
                pan = state["servo"][0]
                if abs(target_pan - pan) > 5:
                    pan += 2 if target_pan > pan else -2
                    state["servo"][0] = int(round(max(0, min(180, pan))))

                d = max(200, min(3000, state["sim"]["dist"]))
                target_tilt = 120 + (d - 200) * (60 - 120) / (3000 - 200)
                tilt = state["servo"][1]
                if abs(target_tilt - tilt) >= 1:
                    tilt += 1 if target_tilt > tilt else -1
                    state["servo"][1] = int(round(max(0, min(180, tilt))))

            return self._send(
                json.dumps({"pan": state["servo"][0], "tilt": state["servo"][1]}),
                "application/json",
            )

        if path == "/servo":
            ch = int(a.get("ch", 0))
            angle = max(0, min(180, int(a.get("angle", 90))))
            state["servo"][ch] = angle
            state["mode"] = "manual"
            print(f"[servo] ch={ch} ({CHANNELS.get(ch, '?')}) angle={angle}")
            return self._send("OK", "text/plain")

        if path == "/servo/angles":
            return self._send(
                json.dumps(
                    {
                        "pan": state["servo"][0],
                        "tilt": state["servo"][1],
                        "armL": state["servo"][2],
                        "armR": state["servo"][3],
                        "hip": state["servo"][4],
                    }
                ),
                "application/json",
            )

        if path == "/led":
            rgb = tuple(max(0, min(255, int(a.get(k, 0)))) for k in ("r", "g", "b"))
            state["led"] = rgb
            print(f"[led] rgb={rgb}")
            return self._send("OK", "text/plain")

        if path == "/brightness":
            state["brightness"] = max(0, min(255, int(a.get("value", 80))))
            print(f"[brightness] {state['brightness']}")
            return self._send("OK", "text/plain")

        if path == "/mode":
            state["mode"] = a.get("set", "auto")
            print(f"[mode] {state['mode']}")
            return self._send("OK", "text/plain")

        if path == "/anim":
            state["anim"]["mode"] = int(a.get("mode", 0))
            state["anim"]["speed"] = int(a.get("speed", 50))
            if all(k in a for k in ("r", "g", "b")):
                state["led"] = tuple(int(a[k]) for k in ("r", "g", "b"))
            print(f"[anim] mode={state['anim']['mode']} speed={state['anim']['speed']} base={state['led']}")
            return self._send("OK", "text/plain")

        if path == "/led/auto":
            print("[led/auto] status colours restored")
            return self._send("OK", "text/plain")

        if path == "/animate":
            print("[animate] startup animation")
            return self._send("OK", "text/plain")

        if path == "/cal/values":
            ch = int(a.get("ch", 0))
            print(f"[cal/values] ch={ch}")
            return self._send(json.dumps(state["cal"].get(ch, {})), "application/json")

        if path == "/cal/channels":
            # Mirrors SERVO_CAL_MASK: all five channels can be calibrated.
            return self._send(json.dumps([0, 1, 2, 3, 4]), "application/json")

        if path == "/servo/channels":
            # Mirrors SERVO_PRESENT_MASK: all five channels drive motion.
            return self._send(json.dumps([0, 1, 2, 3, 4]), "application/json")

        if path == "/cal/pwm":
            ch = int(a.get("ch", 0))
            v = max(100, min(700, int(a.get("v", 375))))
            print(f"[cal/pwm] ch={ch} pwm={v}")
            return self._send("OK", "text/plain")

        if path == "/cal/move":
            ch = int(a.get("ch", 0))
            to = a.get("to", "ctr")
            v = state["cal"][ch].get(to, state["cal"][ch]["ctr"])
            print(f"[cal/move] ch={ch} to={to} pwm={v}")
            return self._send("OK", "text/plain")

        if path == "/cal/save":
            ch = int(a.get("ch", 0))
            which = a.get("which", "ctr")
            v = max(100, min(700, int(a.get("v", 375))))
            key = {"min": "min", "max": "max", "ctr": "ctr"}.get(which, "ctr")
            state["cal"][ch][key] = v
            print(f"[cal/save] ch={ch} {key}={v} -> {state['cal'][ch]}")
            return self._send("OK", "text/plain")

        if path == "/cal/sweep":
            ch = int(a.get("ch", 0))
            print(f"[cal/sweep] ch={ch} range={state['cal'][ch]['min']}-{state['cal'][ch]['max']}")
            return self._send("OK", "text/plain")

        return self._send("Not found", "text/plain", 404)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8765)
    args = ap.parse_args()
    srv = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"PitDroid preview running: http://127.0.0.1:{args.port}/")
    print(f"Calibration page:        http://127.0.0.1:{args.port}/cal")
    print("Ctrl+C to stop.")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
