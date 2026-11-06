#pragma once

// Line-based command protocol over USB serial, used by the web lighting
// editor (tools/lighting-editor) and for bench tuning. One command per line;
// replies start with '@' so they can be told apart from log output.
//
//   HELLO          -> @HELLO pyramid <protocol> keys=<max> bottom=<n> top=<n> fw=<version>
//   GET            -> @PATTERN <json>
//   SET <json>     -> @OK set | @ERR <reason>   apply live (not saved), restart
//   SAVE           -> @OK saved | @ERR <reason> store live pattern in flash
//   REVERT         -> @PATTERN <json>           reload the saved pattern
//   DEFAULT        -> @PATTERN <json>           load the built-in pattern (unsaved)
//   M<n> or <n>    -> @OK motor <n>             restart motor at n %
//   F<n>           -> @OK pwm <hz>              set motor PWM frequency
namespace console {

// Size the receive buffer for SET lines. Call before Serial.begin().
void configure();

// Read and run any complete commands. Call every loop.
void update();

}  // namespace console
