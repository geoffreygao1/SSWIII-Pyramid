#pragma once

#include <Arduino.h>

namespace motor {

// Attach PWM at 0 % duty. Call only once PGND is switched in, since the pin
// is then actively driven.
void begin();

// Set target speed 0..100 %. The output ramps toward it in update().
void setTarget(uint8_t percent);

// Change the PWM frequency on the fly. Returns the frequency actually set
// (0 on failure).
uint32_t setFrequency(uint32_t hz);

// Jump straight to 0 with no ramp.
void stop();

// Stop, detach PWM and leave the pin high-impedance.
void end();

void update();

}  // namespace motor
