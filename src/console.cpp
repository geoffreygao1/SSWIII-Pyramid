#include "console.h"

#include <Arduino.h>

#include "config.h"
#include "leds.h"
#include "motor.h"
#include "pattern.h"

namespace console {

namespace {

constexpr uint8_t kProtocol = 1;
constexpr size_t kLineMax = 4096;  // a full 2 x 16 keyframe SET is ~2.5 kB

char line[kLineMax];
size_t len = 0;
bool overflow = false;

void replyPattern() {
  Serial.print("@PATTERN ");
  Serial.println(pattern::toJson());
}

// Case-insensitive match of the command word at the start of the line.
bool isCommand(const char *s, const char *cmd) {
  const size_t n = strlen(cmd);
  return strncasecmp(s, cmd, n) == 0 && (s[n] == '\0' || s[n] == ' ');
}

void run(char *s) {
  if (isCommand(s, "HELLO")) {
    Serial.printf("@HELLO pyramid %u keys=%u bottom=%u top=%u fw=%s\n", kProtocol,
                  pattern::kMaxKeys, NEOPIXEL_BOTTOM_COUNT,
                  NEOPIXEL_COUNT - NEOPIXEL_BOTTOM_COUNT, FIRMWARE_VERSION);
  } else if (isCommand(s, "GET")) {
    replyPattern();
  } else if (isCommand(s, "SET")) {
    String err;
    if (pattern::fromJson(s + 3, err)) {
      leds::restartPattern();
      Serial.println("@OK set");
    } else {
      Serial.printf("@ERR %s\n", err.c_str());
    }
  } else if (isCommand(s, "SAVE")) {
    Serial.println(pattern::save() ? "@OK saved" : "@ERR could not write flash");
  } else if (isCommand(s, "REVERT")) {
    pattern::begin();
    leds::restartPattern();
    replyPattern();
  } else if (isCommand(s, "DEFAULT")) {
    pattern::loadDefault();
    leds::restartPattern();
    replyPattern();
  } else if ((s[0] == 'F' || s[0] == 'f') && isdigit(s[1])) {
    const uint32_t hz = motor::setFrequency(constrain(atol(s + 1), 10L, 40000L));
    Serial.printf("@OK pwm %lu\n", static_cast<unsigned long>(hz));
  } else if (isdigit(s[0]) || ((s[0] == 'M' || s[0] == 'm') && isdigit(s[1]))) {
    const int percent = constrain(atoi(isdigit(s[0]) ? s : s + 1), 0, 100);
    motor::stop();
    motor::setTarget(percent);
    Serial.printf("@OK motor %d\n", percent);
  } else {
    Serial.printf("@ERR unknown command: %.32s\n", s);
  }
}

}  // namespace

void configure() { Serial.setRxBufferSize(kLineMax); }

void update() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (overflow) {
        Serial.println("@ERR line too long");
      } else if (len > 0) {
        line[len] = '\0';
        run(line);
      }
      len = 0;
      overflow = false;
    } else if (c >= ' ' && c <= '~') {  // drop stray bytes from port open etc.
      if (len < kLineMax - 1) {
        line[len++] = c;
      } else {
        overflow = true;
      }
    }
  }
}

}  // namespace console
