#pragma once

namespace leds {

// Call after the peripheral rail is powered.
void begin();

// Blank the strip and release the data line to high-impedance, ready for
// PGND to be cut (see config.h).
void shutdown();

void update();

// Blink the whole strip red 3 times (driven by update(), non-blocking), then
// return to the normal animation.
void showLowBatteryWarning();

}  // namespace leds
