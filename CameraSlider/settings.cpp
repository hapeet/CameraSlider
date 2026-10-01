#include "settings.h"
#include <Preferences.h>

static constexpr uint32_t CFG_VERSION = 0x43530002;  // při změně struktury zvýšit

Config cfg;
static Preferences prefs;

static void setDefaults() {
  cfg = Config{};
  cfg.version = CFG_VERSION;

  cfg.rangeFrom = 10;   cfg.rangeTo = 500;
  cfg.speedOut = 20;    cfg.speedIn = 20;
  cfg.waitFar = 2;      cfg.waitNear = 2;
  cfg.accel = 50;       cfg.smooth = 5;
  cfg.jogSpeed = 30;
  cfg.cycleInfinite = true;
  cfg.cycleCount = 5;
  cfg.singleOneWay = false;

  cfg.maxPos = 600;
  cfg.steps100 = 500;       // 200 kroků/ot, GT2 20 zubů = 40 mm/ot
  cfg.microsteps = 16;
  cfg.currentMa = 800;
  cfg.stealthChop = true;
  cfg.dirInvert = false;
  cfg.homeSpeed = 15;
  cfg.homeBackoff = 5;
  cfg.hallActiveLow = true;
}

void Settings::begin() {
  prefs.begin("slider", false);
  Config tmp;
  if (prefs.getBytesLength("cfg") == sizeof(Config) &&
      prefs.getBytes("cfg", &tmp, sizeof(Config)) == sizeof(Config) &&
      tmp.version == CFG_VERSION) {
    cfg = tmp;
    cfg.ssid[sizeof(cfg.ssid) - 1] = 0;
    cfg.pass[sizeof(cfg.pass) - 1] = 0;
    Serial.println("[cfg] nastavení načteno");
  } else {
    setDefaults();
    Serial.println("[cfg] výchozí nastavení");
  }
}

void Settings::save() {
  prefs.putBytes("cfg", &cfg, sizeof(Config));
}

void Settings::factoryReset() {
  prefs.clear();
  setDefaults();
}

float Settings::stepsPerMm() {
  return cfg.steps100 * cfg.microsteps / 100.0f;
}

void Settings::toJson(JsonObject o) {
  o["rangeFrom"] = cfg.rangeFrom;
  o["rangeTo"] = cfg.rangeTo;
  o["speedOut"] = cfg.speedOut;
  o["speedIn"] = cfg.speedIn;
  o["waitFar"] = cfg.waitFar;
  o["waitNear"] = cfg.waitNear;
  o["accel"] = cfg.accel;
  o["smooth"] = cfg.smooth;
  o["jogSpeed"] = cfg.jogSpeed;
  o["cycleInfinite"] = cfg.cycleInfinite;
  o["cycleCount"] = cfg.cycleCount;
  o["singleOneWay"] = cfg.singleOneWay;

  o["maxPos"] = cfg.maxPos;
  o["steps100"] = cfg.steps100;
  o["microsteps"] = cfg.microsteps;
  o["currentMa"] = cfg.currentMa;
  o["stealthChop"] = cfg.stealthChop;
  o["dirInvert"] = cfg.dirInvert;
  o["homeSpeed"] = cfg.homeSpeed;
  o["hallActiveLow"] = cfg.hallActiveLow;

  o["stepsPerMm"] = stepsPerMm();
  o["ssid"] = cfg.ssid;
}

// --- čtení s validací ---
static void rdF(JsonObjectConst o, const char *k, float &dst, float lo, float hi) {
  JsonVariantConst v = o[k];
  if (v.is<float>()) dst = constrain(v.as<float>(), lo, hi);
}
static void rdU32(JsonObjectConst o, const char *k, uint32_t &dst, uint32_t lo, uint32_t hi) {
  JsonVariantConst v = o[k];
  if (v.is<float>()) dst = constrain((uint32_t)lroundf(v.as<float>()), lo, hi);
}
static void rdU16(JsonObjectConst o, const char *k, uint16_t &dst, uint16_t lo, uint16_t hi) {
  uint32_t t = dst;
  rdU32(o, k, t, lo, hi);
  dst = (uint16_t)t;
}
static void rdB(JsonObjectConst o, const char *k, bool &dst) {
  JsonVariantConst v = o[k];
  if (v.is<bool>()) dst = v.as<bool>();
}

void Settings::applyMotion(JsonObjectConst o) {
  rdF(o, "rangeFrom", cfg.rangeFrom, 0, cfg.maxPos);
  rdF(o, "rangeTo", cfg.rangeTo, 0, cfg.maxPos);
  rdF(o, "speedOut", cfg.speedOut, 0.01f, 1000);
  rdF(o, "speedIn", cfg.speedIn, 0.01f, 1000);
  rdF(o, "waitFar", cfg.waitFar, 0, 86400);
  rdF(o, "waitNear", cfg.waitNear, 0, 86400);
  rdF(o, "accel", cfg.accel, 1, 10000);
  rdF(o, "smooth", cfg.smooth, 0, 500);
  rdF(o, "jogSpeed", cfg.jogSpeed, 0.1f, 1000);
  rdB(o, "cycleInfinite", cfg.cycleInfinite);
  rdU32(o, "cycleCount", cfg.cycleCount, 1, 1000000);
  rdB(o, "singleOneWay", cfg.singleOneWay);
}

void Settings::applyConfig(JsonObjectConst o, bool &needRehome) {
  uint16_t oldMs = cfg.microsteps;
  bool oldInv = cfg.dirInvert;

  rdF(o, "maxPos", cfg.maxPos, 10, 10000);
  rdF(o, "steps100", cfg.steps100, 1, 1000000);

  uint16_t ms = cfg.microsteps;
  rdU16(o, "microsteps", ms, 1, 256);
  if ((ms & (ms - 1)) == 0) cfg.microsteps = ms;   // jen mocniny 2

  rdU16(o, "currentMa", cfg.currentMa, 100, 2000);
  rdB(o, "stealthChop", cfg.stealthChop);
  rdB(o, "dirInvert", cfg.dirInvert);
  rdF(o, "homeSpeed", cfg.homeSpeed, 0.5f, 200);
  rdB(o, "hallActiveLow", cfg.hallActiveLow);

  cfg.rangeFrom = constrain(cfg.rangeFrom, 0.0f, cfg.maxPos);
  cfg.rangeTo = constrain(cfg.rangeTo, 0.0f, cfg.maxPos);

  needRehome = (oldMs != cfg.microsteps) || (oldInv != cfg.dirInvert);
}
