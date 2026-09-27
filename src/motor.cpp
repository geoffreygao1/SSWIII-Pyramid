#include "motor.h"

#include "config.h"

namespace motor {

namespace {

constexpr uint32_t kMaxDuty = (1u << MOTOR_PWM_BITS) - 1;

uint32_t currentDuty = 0;
uint32_t targetDuty = 0;
uint32_t lastStepMs = 0;
uint32_t kickStartMs = 0;
bool kicking = false;
bool attached = false;

void writeDuty(uint32_t duty) {
  if (!attached) return;
  const uint32_t out = MOTOR_ACTIVE_HIGH ? duty : kMaxDuty - duty;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR_PWM, out);
#else
  ledcWrite(MOTOR_PWM_CHANNEL, out);
#endif
}

}  // namespace

void begin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR_PWM, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
#else
  ledcSetup(MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
  ledcAttachPin(PIN_MOTOR_PWM, MOTOR_PWM_CHANNEL);
#endif
  attached = true;
  stop();
}

void end() {
  stop();
  if (attached) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcDetach(PIN_MOTOR_PWM);
#else
    ledcDetachPin(PIN_MOTOR_PWM);
#endif
    attached = false;
  }
  pinMode(PIN_MOTOR_PWM, INPUT);
}

void setTarget(uint8_t percent) {
  if (percent > 100) percent = 100;
  const uint32_t now = millis();
  if (currentDuty == targetDuty) lastStepMs = now;  // start ramp from now
  targetDuty = kMaxDuty * percent / 100;

  // Starting from standstill: kick hard, then ramp down to the target.
  constexpr uint32_t kickDuty = kMaxDuty * MOTOR_KICK_PERCENT / 100;
  if (currentDuty == 0 && targetDuty > 0 && kickDuty > targetDuty) {
    currentDuty = kickDuty;
    writeDuty(currentDuty);
    kicking = true;
    kickStartMs = now;
  }
}

uint32_t setFrequency(uint32_t hz) {
  if (!attached) return 0;
  uint32_t actual;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  actual = ledcChangeFrequency(PIN_MOTOR_PWM, hz, MOTOR_PWM_BITS) ? hz : 0;
#else
  actual = static_cast<uint32_t>(ledcChangeFrequency(MOTOR_PWM_CHANNEL, hz, MOTOR_PWM_BITS));
#endif
  writeDuty(currentDuty);
  return actual;
}

void stop() {
  currentDuty = targetDuty = 0;
  kicking = false;
  writeDuty(0);
}

void update() {
  const uint32_t now = millis();
  if (kicking) {
    if (now - kickStartMs < MOTOR_KICK_MS) return;
    kicking = false;
    lastStepMs = now;
  }

  if (currentDuty == targetDuty) return;

  // One duty step per (ramp time / full scale), at least 1 ms apart.
  const uint32_t stepMs = max<uint32_t>(1, MOTOR_RAMP_MS / kMaxDuty);
  if (now - lastStepMs < stepMs) return;
  const uint32_t steps = max<uint32_t>(1, (now - lastStepMs) * kMaxDuty / MOTOR_RAMP_MS);
  lastStepMs = now;

  if (currentDuty < targetDuty) {
    currentDuty = min(currentDuty + steps, targetDuty);
  } else {
    currentDuty = (currentDuty - targetDuty > steps) ? currentDuty - steps : targetDuty;
  }
  writeDuty(currentDuty);
}

}  // namespace motor
