#pragma once

#include <Arduino.h>

namespace battery {

// Configure the ADC and take an initial reading.
void begin();

// Sample periodically and track the low-battery condition. Call every loop.
void update();

// Filtered battery voltage in millivolts.
uint32_t millivolts();

// False when the reading is too low to be a real LiPo (USB power only, or a
// sense fault). No warning is raised then.
bool present();

// True once the battery has stayed below the low threshold for the hold
// time; clears again once it recovers above BATTERY_RECOVER_MV.
bool isLow();

}  // namespace battery
