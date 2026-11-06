#include "pattern.h"

#include <ArduinoJson.h>
#include <Preferences.h>

#include "config.h"

namespace pattern {

namespace {

constexpr char kNamespace[] = "pyramid";
constexpr char kKey[] = "pattern";
constexpr char kTrackNames[TrackCount][7] = {"bottom", "top"};

Pattern live;

// Close to the original look: bottom green breathing between bright and a
// dim, washed-out green; top solid dim red. Colours are as seen (gamma is
// applied on output).
void setDefault(Pattern &p) {
  p.brightness = NEOPIXEL_BRIGHTNESS;
  p.count[Bottom] = 2;
  p.keys[Bottom][0] = {0x38, 0x49, 0x38, true, 5750, 0};
  p.keys[Bottom][1] = {0x00, 0xC2, 0x00, true, 5750, 0};
  p.count[Top] = 1;
  p.keys[Top][0] = {0x88, 0x00, 0x00, false, 0, 0};
}

bool parseHex(const char *s, Keyframe &k) {
  if (!s || s[0] != '#' || strlen(s) != 7) return false;
  char *end;
  const uint32_t v = strtoul(s + 1, &end, 16);
  if (*end != '\0') return false;
  k.r = v >> 16;
  k.g = v >> 8;
  k.b = v;
  return true;
}

uint8_t lerp8(uint8_t a, uint8_t b, uint32_t t16) {  // t16 in 0..65535
  return a + ((int32_t)(b - a) * (int32_t)t16) / 65535;
}

}  // namespace

void begin() {
  setDefault(live);
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) return;  // nothing saved yet
  const String json = prefs.getString(kKey, "");
  prefs.end();
  if (json.isEmpty()) return;
  String err;
  if (!fromJson(json.c_str(), err)) {
    Serial.printf("Saved pattern invalid (%s), using default\n", err.c_str());
  }
}

const Pattern &current() { return live; }

bool fromJson(const char *json, String &err) {
  JsonDocument doc;
  const DeserializationError de = deserializeJson(doc, json);
  if (de) {
    err = String("bad JSON: ") + de.c_str();
    return false;
  }

  Pattern p;
  p.brightness = constrain(doc["brightness"] | (int)NEOPIXEL_BRIGHTNESS, 1, 255);
  for (uint8_t t = 0; t < TrackCount; t++) {
    JsonArrayConst arr = doc[kTrackNames[t]];
    if (arr.isNull()) {
      err = String("missing \"") + kTrackNames[t] + "\"";
      return false;
    }
    if (arr.size() > kMaxKeys) {
      err = String(kTrackNames[t]) + ": more than " + kMaxKeys + " keyframes";
      return false;
    }
    p.count[t] = arr.size();
    uint8_t i = 0;
    for (JsonObjectConst o : arr) {
      Keyframe &k = p.keys[t][i];
      if (!parseHex(o["c"].as<const char *>(), k)) {
        err = String(kTrackNames[t]) + "[" + i + "]: colour must be \"#rrggbb\"";
        return false;
      }
      k.fadeMs = min<uint32_t>(o["fade"] | 0u, kMaxTimeMs);
      k.holdMs = min<uint32_t>(o["hold"] | 0u, kMaxTimeMs);
      k.smooth = strcmp(o["ease"] | "linear", "smooth") == 0;
      i++;
    }
  }
  live = p;
  return true;
}

String toJson() {
  JsonDocument doc;
  doc["v"] = 1;
  doc["brightness"] = live.brightness;
  for (uint8_t t = 0; t < TrackCount; t++) {
    JsonArray arr = doc[kTrackNames[t]].to<JsonArray>();
    for (uint8_t i = 0; i < live.count[t]; i++) {
      const Keyframe &k = live.keys[t][i];
      char hex[8];
      snprintf(hex, sizeof(hex), "#%02x%02x%02x", k.r, k.g, k.b);
      JsonObject o = arr.add<JsonObject>();
      o["c"] = hex;
      o["fade"] = k.fadeMs;
      o["hold"] = k.holdMs;
      o["ease"] = k.smooth ? "smooth" : "linear";
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}

bool save() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  const String json = toJson();
  const bool ok = prefs.putString(kKey, json) == json.length();
  prefs.end();
  return ok;
}

void loadDefault() { setDefault(live); }

void colorAt(Track track, uint32_t elapsedMs, uint8_t &r, uint8_t &g, uint8_t &b) {
  const uint8_t n = live.count[track];
  const Keyframe *keys = live.keys[track];
  r = g = b = 0;
  if (n == 0) return;

  uint32_t total = 0;
  for (uint8_t i = 0; i < n; i++) total += keys[i].fadeMs + keys[i].holdMs;
  const Keyframe &last = keys[n - 1];
  if (total == 0) {
    r = last.r, g = last.g, b = last.b;
    return;
  }

  uint32_t t = elapsedMs % total;
  for (uint8_t i = 0; i < n; i++) {
    const Keyframe &k = keys[i];
    const Keyframe &prev = keys[i == 0 ? n - 1 : i - 1];
    if (t < k.fadeMs) {
      uint32_t t16 = (uint64_t)t * 65535 / k.fadeMs;
      if (k.smooth) {  // smoothstep: 3t^2 - 2t^3
        const uint64_t x = t16;
        t16 = (3 * x * x * 65535 - 2 * x * x * x) / (65535ULL * 65535ULL);
      }
      r = lerp8(prev.r, k.r, t16);
      g = lerp8(prev.g, k.g, t16);
      b = lerp8(prev.b, k.b, t16);
      return;
    }
    t -= k.fadeMs;
    if (t < k.holdMs) {
      r = k.r, g = k.g, b = k.b;
      return;
    }
    t -= k.holdMs;
  }
  r = last.r, g = last.g, b = last.b;
}

}  // namespace pattern
