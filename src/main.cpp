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
#include "leds.h"
#include "motor.h"
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

// Debug commands over USB serial, for tuning without reflashing:
//   "18"    restart the motor from standstill at 18 %
//   "f1000" set the motor PWM frequency to 1000 Hz
static void handleSerialCommands() {
  static uint32_t value = 0;
  static uint8_t digits = 0;
  static bool freq = false;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c >= '0' && c <= '9') {
      if (digits < 6) value = value * 10 + (c - '0');
      digits++;
    } else if (c == 'f' || c == 'F') {
      freq = true;
    } else if (c == '\n' || c == '\r') {
      if (digits > 0 && freq) {
        const uint32_t actual = motor::setFrequency(constrain(value, 10u, 40000u));
        Serial.printf("Motor PWM -> %lu Hz\n", static_cast<unsigned long>(actual));
      } else if (digits > 0) {
        const int percent = constrain(value, 0u, 100u);
        motor::stop();
        motor::setTarget(percent);
        Serial.printf("Motor -> %d%%\n", percent);
      }
      value = 0;
      digits = 0;
      freq = false;
    }
    // Anything else (e.g. stray bytes when the port opens) is ignored.
  }
}

void setup() {
  power::begin();

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

  handleSerialCommands();
  leds::update();
  motor::update();
}
