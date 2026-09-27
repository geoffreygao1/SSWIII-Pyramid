#pragma once

namespace power {

// Release pin holds left over from the previous sleep and configure outputs.
// Call first thing in setup().
void begin();

void peripheralsOn();
void peripheralsOff();

// Blocking: wait for the switch to read the same for the debounce time and
// return whether it is ON. Used at boot.
bool switchSettledOn();

// Non-blocking debounced switch state. Call every loop.
bool switchOn();

// Shut everything down, latch outputs off, and enter deep sleep until the
// switch is turned ON (the chip then reboots into setup()).
[[noreturn]] void enterDeepSleep();

}  // namespace power
