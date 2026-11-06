#include "leds.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "pattern.h"

namespace leds {

namespace {

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

constexpr uint32_t kFrameMs = 20;

// Low-battery warning: 3 red blinks (on/off phases), then back to the pattern.
constexpr uint8_t  kWarnPhases = 6;
constexpr uint32_t kWarnPhaseMs = 250;

bool begun = false;
bool warning = false;
uint32_t warnStartMs = 0;
uint8_t warnPhase = 0xFF;
uint32_t lastFrameMs = 0;
uint32_t patternStartMs = 0;

// Pattern colours are picked on a screen, so apply gamma to make the LEDs
// look like the picker, then the global brightness.
uint32_t trackColor(pattern::Track track, uint32_t elapsedMs) {
  uint8_t r, g, b;
  pattern::colorAt(track, elapsedMs, r, g, b);
  const uint16_t bri = pattern::current().brightness;
  return strip.Color(Adafruit_NeoPixel::gamma8(r) * bri / 255,
                     Adafruit_NeoPixel::gamma8(g) * bri / 255,
                     Adafruit_NeoPixel::gamma8(b) * bri / 255);
}

}  // namespace

void begin() {
  strip.begin();
  strip.clear();
  strip.show();
  begun = true;
  restartPattern();
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

void restartPattern() {
  patternStartMs = millis();
  lastFrameMs = patternStartMs - kFrameMs;  // draw on the next update()
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
    warning = false;  // done: fall through and resume the pattern
  }

  if (now - lastFrameMs < kFrameMs) return;
  lastFrameMs = now;

  const uint32_t elapsed = now - patternStartMs;
  const uint32_t bottom = trackColor(pattern::Bottom, elapsed);
  const uint32_t top = trackColor(pattern::Top, elapsed);
  for (uint16_t i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, i < NEOPIXEL_BOTTOM_COUNT ? bottom : top);
  }
  strip.show();
}

void showLowBatteryWarning() {
  warning = true;
  warnStartMs = millis();
  warnPhase = 0xFF;
}

}  // namespace leds
