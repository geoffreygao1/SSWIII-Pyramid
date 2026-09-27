#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "config.h"
#include "leds.h"
#include "motor.h"

namespace power {

namespace {

constexpr uint8_t kPeriphOffLevel = PERIPH_ON_LEVEL == HIGH ? LOW : HIGH;

bool stableOn = false;
bool lastReading = false;
uint32_t lastChangeMs = 0;

bool readOn() { return digitalRead(PIN_SLEEP_SWITCH) == SWITCH_ON_LEVEL; }

void setFloating(uint8_t pin) {
  pinMode(pin, INPUT);
  gpio_pullup_dis(static_cast<gpio_num_t>(pin));
  gpio_pulldown_dis(static_cast<gpio_num_t>(pin));
}

// Pin config is applied while the pad is still latched from the last sleep,
// then the latch is released, so the pin never passes through another state
// (GPIO43 would otherwise revert to UART TX, which idles HIGH).
void releaseOff(uint8_t pin, uint8_t level) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_dis(static_cast<gpio_num_t>(pin));
}

void releaseFloating(uint8_t pin) {
  setFloating(pin);
  gpio_hold_dis(static_cast<gpio_num_t>(pin));
}

void holdOff(uint8_t pin, uint8_t level) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_en(static_cast<gpio_num_t>(pin));
}

void holdFloating(uint8_t pin) {
  setFloating(pin);
  gpio_hold_en(static_cast<gpio_num_t>(pin));
}

}  // namespace

void begin() {
  // Q2 off; LED data and motor gate float until PGND is switched in.
  releaseOff(PIN_PERIPH_EN, kPeriphOffLevel);
  releaseFloating(PIN_MOTOR_PWM);
  releaseFloating(PIN_NEOPIXEL);
  gpio_deep_sleep_hold_dis();

  rtc_gpio_deinit(static_cast<gpio_num_t>(PIN_SLEEP_SWITCH));
  setFloating(PIN_SLEEP_SWITCH);  // SPDT drives it both ways
}

void peripheralsOn() {
  digitalWrite(PIN_PERIPH_EN, PERIPH_ON_LEVEL);
  delay(PERIPH_SETTLE_MS);
}

void peripheralsOff() { digitalWrite(PIN_PERIPH_EN, kPeriphOffLevel); }

bool switchSettledOn() {
  bool reading = readOn();
  uint32_t since = millis();
  while (millis() - since < SWITCH_DEBOUNCE_MS) {
    const bool now = readOn();
    if (now != reading) {
      reading = now;
      since = millis();
    }
    delay(1);
  }
  stableOn = lastReading = reading;
  lastChangeMs = millis();
  return reading;
}

bool switchOn() {
  const bool reading = readOn();
  const uint32_t now = millis();
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = now;
  }
  if (now - lastChangeMs >= SWITCH_DEBOUNCE_MS) stableOn = reading;
  return stableOn;
}

void enterDeepSleep() {
  Serial.println("Sleeping until switch ON");
  Serial.flush();

  // Quiet the loads and float their signal pins before cutting PGND.
  motor::end();
  leds::shutdown();
  peripheralsOff();

  // Latch pin states through sleep: Q2 gate off, PGND-side pins floating.
  holdOff(PIN_PERIPH_EN, kPeriphOffLevel);
  holdFloating(PIN_MOTOR_PWM);
  holdFloating(PIN_NEOPIXEL);
  gpio_deep_sleep_hold_en();

  // ext0 is level-triggered. If the switch is already back ON the chip wakes
  // immediately and setup() re-evaluates, which is fine.
  const auto sw = static_cast<gpio_num_t>(PIN_SLEEP_SWITCH);
  rtc_gpio_init(sw);
  rtc_gpio_set_direction(sw, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_dis(sw);
  rtc_gpio_pulldown_dis(sw);
  esp_sleep_enable_ext0_wakeup(sw, SWITCH_ON_LEVEL);

  esp_deep_sleep_start();
}

}  // namespace power
