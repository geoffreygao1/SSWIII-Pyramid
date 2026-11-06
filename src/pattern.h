#pragma once

#include <Arduino.h>

// Keyframe lighting pattern. The bottom and top LED sets each play their own
// looping track. Keyframe i fades from keyframe i-1's colour (the last one
// for i = 0) over fadeMs, then holds for holdMs. A track with a single
// keyframe is a solid colour; an empty track is off.
namespace pattern {

constexpr uint8_t kMaxKeys = 16;
constexpr uint32_t kMaxTimeMs = 600000;  // per fade / hold

enum Track : uint8_t { Bottom = 0, Top = 1, TrackCount = 2 };

struct Keyframe {
  uint8_t r, g, b;
  bool smooth;  // eased (smoothstep) fade instead of linear
  uint32_t fadeMs;
  uint32_t holdMs;
};

struct Pattern {
  uint8_t brightness;  // global scale 1..255
  uint8_t count[TrackCount];
  Keyframe keys[TrackCount][kMaxKeys];
};

// Load the saved pattern from flash, or the default if none is saved.
void begin();

const Pattern &current();

// Replace the live pattern from JSON (not saved). On failure the live
// pattern is unchanged and err describes why.
bool fromJson(const char *json, String &err);
String toJson();

// Persist the live pattern to flash.
bool save();

// Replace the live pattern with the built-in default (not saved).
void loadDefault();

// Colour of a track `elapsedMs` into its loop, as raw 0..255 RGB (before
// gamma and brightness).
void colorAt(Track track, uint32_t elapsedMs, uint8_t &r, uint8_t &g, uint8_t &b);

}  // namespace pattern
