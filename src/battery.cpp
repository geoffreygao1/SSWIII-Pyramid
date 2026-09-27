#include "battery.h"

#include "config.h"

namespace battery {

namespace {

constexpr uint8_t  kSamplesPerRead = 16;
constexpr uint32_t kSampleMs = 100;

uint32_t filteredMv = 0;
uint32_t lastSampleMs = 0;
uint32_t lastLogMs = 0;
uint32_t lowSinceMs = 0;
bool low = false;

// Averaged, calibrated voltage at the sense pin.
uint32_t readPinMv() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < kSamplesPerRead; i++) {
    sum += analogReadMilliVolts(PIN_BATTERY_SENSE);
  }
  return sum / kSamplesPerRead;
}

uint32_t toBatteryMv(uint32_t pinMv) {
  return pinMv * (BATTERY_R_TOP_K + BATTERY_R_BOTTOM_K) / BATTERY_R_BOTTOM_K;
}

}  // namespace

void begin() {
  // 11 dB attenuation covers ~0..3.1 V at the pin (4.2 V battery -> 2.1 V).
  analogSetPinAttenuation(PIN_BATTERY_SENSE, ADC_11db);
  const uint32_t pinMv = readPinMv();
  filteredMv = toBatteryMv(pinMv);
  lastSampleMs = lastLogMs = millis();
  lowSinceMs = 0;
  low = false;

  Serial.printf("Battery sense: pin %lu mV -> battery %lu mV%s\n",
                static_cast<unsigned long>(pinMv),
                static_cast<unsigned long>(filteredMv),
                present() ? "" : " (no battery detected, warning disabled)");
}

void update() {
  const uint32_t now = millis();
  if (now - lastSampleMs < kSampleMs) return;
  lastSampleMs = now;

  // Exponential moving average, weight 1/8, to smooth motor noise.
  const uint32_t pinMv = readPinMv();
  filteredMv = (filteredMv * 7 + toBatteryMv(pinMv)) / 8;

  if (present() && filteredMv < BATTERY_LOW_MV) {
    if (lowSinceMs == 0) lowSinceMs = now;
    if (now - lowSinceMs >= BATTERY_LOW_HOLD_MS) low = true;
  } else {
    lowSinceMs = 0;
    if (!present() || filteredMv > BATTERY_RECOVER_MV) low = false;
  }

  if (now - lastLogMs >= BATTERY_LOG_MS) {
    lastLogMs = now;
    Serial.printf("Battery: %lu mV (pin %lu mV)\n", static_cast<unsigned long>(filteredMv),
                  static_cast<unsigned long>(pinMv));
  }
}

uint32_t millivolts() { return filteredMv; }

bool present() { return filteredMv >= BATTERY_PRESENT_MIN_MV; }

bool isLow() { return BATTERY_WARNING_ENABLED && low; }

}  // namespace battery
