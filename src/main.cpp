// SSWIII Pyramid toy — XIAO ESP32S3
//
// A latching switch selects run or deep sleep. Every wake is a reset, so
// setup() decides what to do from the switch position. A low battery only
// blinks the LEDs red as a warning.
//
// Q2 (GATE) switches PGND for the 5 V boost, LEDs and motor. It goes on
// before the LED/motor pins are driven and off after they are released.

#include <Arduino.h>
#include <esp_sleep.h>

#include "battery.h"
#include "config.h"
#include "console.h"
#include "leds.h"
#include "motor.h"
#include "pattern.h"
#include "power.h"

// True when a USB host is attached (the battery is then being charged).
static bool usbConnected() {
#if ARDUINO_USB_MODE
  // isPlugged() starts out true and only drops after a few ms without USB
  // SOF frames, so let it settle after boot.
  while (millis() < 20) delay(1);
  return Serial.isPlugged();
#else
  return false;
#endif
}

// Blink red when the battery goes low, and again every repeat interval while
// it stays low. Warning only: everything keeps running.
static void checkBattery() {
  static bool warned = false;
  static uint32_t lastWarnMs = 0;
  if (!battery::isLow()) {
    warned = false;
    return;
  }
  const uint32_t now = millis();
  if (!warned || now - lastWarnMs >= BATTERY_WARN_REPEAT_MS) {
    Serial.printf("Low battery (%lu mV)\n",
                  static_cast<unsigned long>(battery::millivolts()));
    leds::showLowBatteryWarning();
    warned = true;
    lastWarnMs = now;
  }
}

void setup() {
  power::begin();

  console::configure();
  Serial.begin(115200);
  if (usbConnected()) {
    const uint32_t t0 = millis();
    while (!Serial && millis() - t0 < USB_SERIAL_WAIT_MS) delay(10);
  }

  const bool on = power::switchSettledOn();
  Serial.printf("Boot: wake cause %d, switch %s\n",
                static_cast<int>(esp_sleep_get_wakeup_cause()), on ? "ON" : "OFF");
  if (!on) {
    power::enterDeepSleep();
  }

  battery::begin();
  pattern::begin();

  power::peripheralsOn();
  leds::begin();
  motor::begin();
  motor::setTarget(MOTOR_RUN_PERCENT);
}

void loop() {
  if (!power::switchOn()) {
    power::enterDeepSleep();
  }

  battery::update();
  checkBattery();

  console::update();
  leds::update();
  motor::update();
}
