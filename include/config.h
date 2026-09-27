#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pin map — XIAO ESP32S3, per SWIII "Schematic1 v2" (Main Board v2)
//
// XIAO pad -> GPIO:
//   D0=1  D1=2  D2=3  D3=4  D4=5  D5=6  D6=43  D7=44  D8=7  D9=8  D10=9
//
// Power topology: Q2 (N-ch, net GATE) switches PGND, the ground of the 5 V
// boost (U1), all WS2812s and the motor MOSFET Q1. With Q2 off PGND floats
// up toward BAT+, so LED1 and MTR_GATE must be left high-impedance then —
// driving either LOW would give PGND a ground path through the GPIO (via
// the first LED's input diode, or via R3/R4) and leak current in sleep.
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_BATTERY_SENSE = 4;   // D3 / A3 — BAT_SENSE, R1/R2 220k/220k
constexpr uint8_t PIN_NEOPIXEL      = 5;   // D4 — LED1, WS2812 data
constexpr uint8_t PIN_PERIPH_EN     = 6;   // D5 — GATE, Q2 (10k pull-down R6)
constexpr uint8_t PIN_MOTOR_PWM     = 43;  // D6 — MTR_GATE, Q1 (10k R4 to PGND).
                                           // Also U0TXD: toggled by the ROM boot log.
constexpr uint8_t PIN_SLEEP_SWITCH  = 9;   // D10 — SLEEP, SPDT switch to 3V3 / GND.
                                           // Must be RTC-capable (GPIO 0..21).

// Logic level that turns each MOSFET on. Both are N-channel low-side.
constexpr uint8_t PERIPH_ON_LEVEL = HIGH;
constexpr bool    MOTOR_ACTIVE_HIGH = true;

// SLEEP level that means "run". The SPDT switch drives the pin both ways, so
// no pull resistor is used. HIGH = switch pushed in / latched = run.
constexpr uint8_t  SWITCH_ON_LEVEL    = HIGH;
constexpr uint32_t SWITCH_DEBOUNCE_MS = 50;

// ---------------------------------------------------------------------------
// NeoPixels
// ---------------------------------------------------------------------------
constexpr uint16_t NEOPIXEL_COUNT = 8;
// Global scale on top of the animation's own levels. 255 = unscaled, which
// matches the previous version (its colours already top out at ~125).
constexpr uint8_t  NEOPIXEL_BRIGHTNESS = 255;

// Time for the motor/LED rail to settle after the peripheral MOSFET turns on.
constexpr uint32_t PERIPH_SETTLE_MS = 20;

// ---------------------------------------------------------------------------
// Motor PWM
// ---------------------------------------------------------------------------
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;  // above audible range
constexpr uint8_t  MOTOR_PWM_BITS    = 10;     // duty 0..1023
constexpr uint8_t  MOTOR_PWM_CHANNEL = 0;      // LEDC channel (Arduino-ESP32 2.x)
constexpr uint8_t  MOTOR_RUN_PERCENT = 18;     // speed after wake-up (15 unreliable, 12 stalls)
constexpr uint32_t MOTOR_RAMP_MS     = 800;    // time for a full-scale speed change
// Low run speeds can't overcome static friction from standstill, so a start
// from 0 briefly drives this hard, then ramps down to the run speed.
// 0 (or anything <= the run speed) disables the kick.
constexpr uint8_t  MOTOR_KICK_PERCENT = 0;
constexpr uint32_t MOTOR_KICK_MS      = 150;

// ---------------------------------------------------------------------------
// Battery (1S LiPo)
// ---------------------------------------------------------------------------
constexpr uint32_t BATTERY_R_TOP_K    = 220;  // battery -> sense pin
constexpr uint32_t BATTERY_R_BOTTOM_K = 220;  // sense pin -> GND

// Low battery only triggers a warning (red blinks); the toy keeps running.
// Protecting the cell from over-discharge is left to the LiPo's own
// protection circuit.
constexpr bool     BATTERY_WARNING_ENABLED = true;
// Readings below this can't be a working LiPo: treat as "no battery"
// (running from USB) or a sense fault, and don't warn.
constexpr uint32_t BATTERY_PRESENT_MIN_MV = 2500;
constexpr uint32_t BATTERY_LOW_MV      = 3300;   // warn below this
constexpr uint32_t BATTERY_RECOVER_MV  = 3400;   // clear the warning above this
constexpr uint32_t BATTERY_LOW_HOLD_MS = 5000;   // must stay low this long (ignores motor sag)
constexpr uint32_t BATTERY_WARN_REPEAT_MS = 60000;  // re-blink interval while low
constexpr uint32_t BATTERY_LOG_MS      = 5000;   // serial report interval

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------
// With USB plugged in, wait up to this long at boot for a serial monitor to
// attach so the boot log isn't lost. No effect when running on battery.
constexpr uint32_t USB_SERIAL_WAIT_MS = 2000;
