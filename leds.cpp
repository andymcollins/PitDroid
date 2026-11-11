#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "leds.h"

static uint8_t  ledMode    = LED_SOLID;
static uint8_t  ledSpeed   = 50;
static uint8_t  baseR = 0, baseG = 0, baseB = 255;
static bool     manualMode = false;

static uint32_t lastFrameMs = 0;
static uint32_t frame       = 0;
static uint32_t buf[LED_COUNT];

// ---------------------------------------------------------------------------
// Colour helpers (buf holds 0x00RRGGBB, independent of NeoPixel byte order)
// ---------------------------------------------------------------------------
static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static inline uint32_t scaleColor(uint32_t c, uint8_t s) {
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8) & 0xFF;
  uint8_t b = c & 0xFF;
  return rgb((uint8_t)((uint16_t)r * s / 255),
             (uint8_t)((uint16_t)g * s / 255),
             (uint8_t)((uint16_t)b * s / 255));
}

static uint32_t hsv(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t  region    = h / 10923;          // 65536 / 6
  uint16_t remainder = h % 10923;
  uint8_t  p = (uint8_t)((uint32_t)v * (255 - s) / 255);
  uint8_t  q = (uint8_t)((uint32_t)v * (255 - ((uint32_t)s * remainder / 10923)) / 255);
  uint8_t  t = (uint8_t)((uint32_t)v * (255 - ((uint32_t)s * (10923 - remainder) / 10923)) / 255);
  switch (region) {
    case 0:  return rgb(v, t, p);
    case 1:  return rgb(q, v, p);
    case 2:  return rgb(p, v, t);
    case 3:  return rgb(p, q, v);
    case 4:  return rgb(t, p, v);
    default: return rgb(v, p, q);
  }
}

// ---------------------------------------------------------------------------
// Patterns
// ---------------------------------------------------------------------------
static void renderSolid() {
  uint32_t c = rgb(baseR, baseG, baseB);
  for (int i = 0; i < LED_COUNT; i++) buf[i] = c;
}

static void renderRainbow() {
  for (int i = 0; i < LED_COUNT; i++) {
    uint16_t hue = (uint16_t)((uint32_t)i * 65536UL / LED_COUNT + frame * 256UL);
    buf[i] = hsv(hue, 255, 255);
  }
}

static void renderChase() {
  uint32_t dim = scaleColor(rgb(baseR, baseG, baseB), 20);
  for (int i = 0; i < LED_COUNT; i++) buf[i] = dim;
  buf[frame % LED_COUNT] = rgb(baseR, baseG, baseB);
}

static void renderComet() {
  const uint8_t tail = 8;
  int pos = frame % LED_COUNT;
  for (int i = 0; i < LED_COUNT; i++) {
    int d = (pos - i + LED_COUNT) % LED_COUNT;
    if (d < tail) {
      uint8_t level = 255 - (uint8_t)(d * 255 / tail);
      buf[i] = scaleColor(rgb(baseR, baseG, baseB), level);
    } else {
      buf[i] = 0;
    }
  }
}

static void renderBreathe() {
  uint32_t phase = frame % 120;
  uint8_t level = (phase < 60) ? (uint8_t)(phase * 255 / 60)
                               : (uint8_t)((120 - phase) * 255 / 60);
  if (level < 8) level = 8;
  uint32_t c = scaleColor(rgb(baseR, baseG, baseB), level);
  for (int i = 0; i < LED_COUNT; i++) buf[i] = c;
}

static void renderSparkle() {
  for (int i = 0; i < LED_COUNT; i++) buf[i] = scaleColor(buf[i], 180);
  for (int n = 0; n < 3; n++) {
    int i = random(LED_COUNT);
    buf[i] = hsv((uint16_t)random(65536), 255, 255);
  }
}

static void renderScanner() {
  const uint8_t tail = 6;
  int period = 2 * (LED_COUNT - 1);
  int p = frame % period;
  int pos = (p < LED_COUNT) ? p : (period - p);
  for (int i = 0; i < LED_COUNT; i++) {
    int d = abs(i - pos);
    if (d < tail) {
      uint8_t level = 255 - (uint8_t)(d * 255 / tail);
      buf[i] = scaleColor(rgb(baseR, baseG, baseB), level);
    } else {
      buf[i] = 0;
    }
  }
}

static void renderTheater() {
  uint32_t full = rgb(baseR, baseG, baseB);
  uint32_t dim  = scaleColor(full, 25);
  for (int i = 0; i < LED_COUNT; i++) {
    buf[i] = ((i + frame) % 3 == 0) ? full : dim;
  }
}

// Ring lengths are unequal, so map each ring to its own slice of the strip.
static const uint16_t RING_START[RING_COUNT] = {
  0,
  RING0_LEDS,
  RING0_LEDS + RING1_LEDS
};
static const uint16_t RING_LEN[RING_COUNT] = {
  RING0_LEDS,
  RING1_LEDS,
  RING2_LEDS
};

static void renderRings() {
  for (uint8_t ring = 0; ring < RING_COUNT; ring++) {
    uint16_t hue = (uint16_t)((uint32_t)ring * (65536UL / RING_COUNT) + frame * 180UL);
    uint32_t c = hsv(hue, 255, 255);
    for (uint16_t i = RING_START[ring]; i < RING_START[ring] + RING_LEN[ring]; i++) {
      buf[i] = c;
    }
  }
}

// Every pixel picks a new random hue each frame.
static void renderRandom() {
  for (int i = 0; i < LED_COUNT; i++) {
    buf[i] = hsv((uint16_t)random(65536), 255, 255);
  }
}

// Classic fire simulation: heat rises along the strip, cools and flickers.
static void renderFire() {
  static uint8_t heat[LED_COUNT];
  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t cool = (uint8_t)random(60);
    heat[i] = (heat[i] > cool) ? heat[i] - cool : 0;
  }
  for (int i = LED_COUNT - 1; i >= 2; i--) {
    heat[i] = (uint8_t)(((uint16_t)heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3);
  }
  if (random(100) < 60) {
    int y = (int)random(3);
    int v = heat[y] + 160 + (int)random(96);
    heat[y] = (uint8_t)(v > 255 ? 255 : v);
  }
  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t t = heat[i];
    if (t < 85)       buf[i] = rgb(t * 3, 0, 0);
    else if (t < 170) buf[i] = rgb(255, (t - 85) * 3, 0);
    else              buf[i] = rgb(255, 255, (t - 170) * 3);
  }
}

// Base colour fills the strip, then drains from the far end.
static void renderWipe() {
  int cycle = LED_COUNT + 6;
  int p = frame % (cycle * 2);
  int len = (p < cycle) ? p : (cycle * 2 - p);
  if (len > LED_COUNT) len = LED_COUNT;
  uint32_t c = rgb(baseR, baseG, baseB);
  for (int i = 0; i < LED_COUNT; i++) {
    buf[i] = (i < len) ? c : 0;
  }
}

// Double-thump heartbeat in the base colour.
static void renderPulse() {
  uint32_t p = frame % 48;
  uint8_t level;
  if (p < 4)        level = (uint8_t)(80 + p * 44);
  else if (p < 10)  level = (uint8_t)(255 - (p - 4) * 30);
  else if (p < 14)  level = (uint8_t)(60 + (p - 10) * 40);
  else if (p < 22)  level = (uint8_t)(220 - (p - 14) * 26);
  else              level = 12;
  uint32_t c = scaleColor(rgb(baseR, baseG, baseB), level);
  for (int i = 0; i < LED_COUNT; i++) buf[i] = c;
}

// Comet whose tail cycles through the full rainbow.
static void renderRainbowComet() {
  const uint8_t tail = 10;
  int pos = frame % LED_COUNT;
  for (int i = 0; i < LED_COUNT; i++) {
    int d = (pos - i + LED_COUNT) % LED_COUNT;
    if (d < tail) {
      uint16_t hue = (uint16_t)(frame * 512UL + (uint32_t)i * 1024UL);
      uint8_t level = (uint8_t)(255 - d * 255 / tail);
      buf[i] = scaleColor(hsv(hue, 255, 255), level);
    } else {
      buf[i] = 0;
    }
  }
}

// Two base-colour dots chase in opposite directions.
static void renderDualChase() {
  uint32_t dim = scaleColor(rgb(baseR, baseG, baseB), 16);
  for (int i = 0; i < LED_COUNT; i++) buf[i] = dim;
  int a = frame % LED_COUNT;
  int b = (LED_COUNT - 1) - a;
  uint32_t c = rgb(baseR, baseG, baseB);
  buf[a] = c;
  buf[b] = c;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ledBegin() {
  pixels.begin();
  pixels.setBrightness(80);
  pixels.clear();
  pixels.show();
  for (int i = 0; i < LED_COUNT; i++) buf[i] = 0;
  lastFrameMs = millis();
}

void ledUpdate() {
  uint16_t interval = map(ledSpeed, 1, 100, 200, 10);
  uint32_t now = millis();
  if (now - lastFrameMs < interval) return;
  lastFrameMs = now;
  frame++;

  switch (ledMode) {
    case LED_RAINBOW: renderRainbow(); break;
    case LED_CHASE:   renderChase();   break;
    case LED_COMET:   renderComet();   break;
    case LED_BREATHE: renderBreathe(); break;
    case LED_SPARKLE: renderSparkle(); break;
    case LED_SCANNER: renderScanner(); break;
    case LED_THEATER: renderTheater(); break;
    case LED_RINGS:   renderRings();   break;
    case LED_RANDOM:  renderRandom();  break;
    case LED_FIRE:    renderFire();    break;
    case LED_WIPE:    renderWipe();    break;
    case LED_PULSE:   renderPulse();   break;
    case LED_RAINBOW_COMET: renderRainbowComet(); break;
    case LED_DUAL_CHASE:    renderDualChase();    break;
    default:          renderSolid();   break;
  }

  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t c = buf[i];
    pixels.setPixelColor(i, pixels.Color((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
  }
  pixels.show();
}

void ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
  baseR = r; baseG = g; baseB = b;
  ledMode = LED_SOLID;
  manualMode = true;
}

void ledSetMode(uint8_t mode) {
  if (mode < LED_MODE_COUNT) {
    ledMode = mode;
    manualMode = true;
  }
}

void ledSetSpeed(uint8_t speed) {
  ledSpeed = constrain(speed, 1, 100);
}

void ledSetBrightness(uint8_t brightness) {
  pixels.setBrightness(brightness);
  pixels.show();
}

void ledSetStatus(uint8_t r, uint8_t g, uint8_t b) {
  if (manualMode) return;
  baseR = r; baseG = g; baseB = b;
  ledMode = LED_SOLID;
}

void ledAuto() {
  manualMode = false;
}

// ---------------------------------------------------------------------------
// Legacy helpers
// ---------------------------------------------------------------------------
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  ledSetColor(r, g, b);
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)  return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if (WheelPos < 170) { WheelPos -= 85; return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3); }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
