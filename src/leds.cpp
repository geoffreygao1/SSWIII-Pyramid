#include "leds.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"

namespace leds {

namespace {

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Lighting from the previous (RP2040 / MicroPython) version:
//   LEDs 0-3 (bottom): green, breathing in brightness and saturation
//   LEDs 4-7 (top):    solid dim red
constexpr uint8_t  kBottomCount = 4;
constexpr uint16_t kBottomHue = 21845;  // green
constexpr uint16_t kTopHue = 0;         // red
constexpr uint8_t  kTopSat = 255;
constexpr uint8_t  kTopVal = 50;

constexpr uint8_t kMinVal = 10;
constexpr uint8_t kMaxVal = 125;
constexpr uint8_t kMinSat = 128;
constexpr uint8_t kMaxSat = 255;
constexpr uint8_t kBreathStep = 1;
constexpr uint32_t kFrameMs = 50;

// Low-battery warning: 3 red blinks (on/off phases), then back to breathing.
constexpr uint8_t  kWarnPhases = 6;
constexpr uint32_t kWarnPhaseMs = 250;

bool begun = false;
bool warning = false;
uint32_t warnStartMs = 0;
uint8_t warnPhase = 0xFF;
uint32_t lastFrameMs = 0;
int16_t val = kMaxVal;
int16_t sat = kMaxSat;
bool valUp = false;
bool satUp = false;

// Bounce between lo and hi, flipping direction at each end.
void breathe(int16_t &x, bool &up, int16_t lo, int16_t hi) {
  if (up) {
    x += kBreathStep;
    if (x >= hi) up = false;
  } else {
    x -= kBreathStep;
    if (x <= lo) up = true;
  }
}

}  // namespace

void begin() {
  strip.begin();
  strip.setBrightness(NEOPIXEL_BRIGHTNESS);
  strip.clear();
  strip.show();
  begun = true;

  val = kMaxVal;
  sat = kMaxSat;
  valUp = satUp = false;
  lastFrameMs = millis();
}

void shutdown() {
  // Only blank if running: show() would otherwise start driving the pin
  // while PGND is floating.
  if (begun) {
    strip.clear();
    strip.show();
    begun = false;
  }
  pinMode(PIN_NEOPIXEL, INPUT);
}

void update() {
  const uint32_t now = millis();

  if (warning) {
    const uint32_t phase = (now - warnStartMs) / kWarnPhaseMs;
    if (phase < kWarnPhases) {
      if (phase != warnPhase) {
        warnPhase = phase;
        if (phase % 2 == 0) {
          strip.fill(strip.Color(60, 0, 0));
        } else {
          strip.clear();
        }
        strip.show();
      }
      return;
    }
    warning = false;  // done: fall through and resume breathing
  }

  if (now - lastFrameMs < kFrameMs) return;
  lastFrameMs = now;

  breathe(val, valUp, kMinVal, kMaxVal);
  breathe(sat, satUp, kMinSat, kMaxSat);

  const uint32_t bottom = strip.ColorHSV(kBottomHue, sat, val);
  const uint32_t top = strip.ColorHSV(kTopHue, kTopSat, kTopVal);
  for (uint16_t i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, i < kBottomCount ? bottom : top);
  }
  strip.show();
}

void showLowBatteryWarning() {
  warning = true;
  warnStartMs = millis();
  warnPhase = 0xFF;
}

}  // namespace leds
