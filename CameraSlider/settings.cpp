#include "settings.h"
#include <Preferences.h>

static constexpr uint32_t CFG_VERSION = 0x43530003;  // při změně struktury zvýšit
static constexpr uint32_t CFG_VERSION_V2 = 0x43530002;
static constexpr size_t CFG_V2_SIZE = offsetof(Config, photoOn);   // v2 = prefix v3

Config cfg;
static Preferences prefs;

static void setPhotoDefaults() {
  cfg.photoOn = false;
  cfg.photoSlow = false;
  cfg.photoBack = false;
  cfg.photoStep = 10;
  cfg.photoSlowSpeed = 2;
  cfg.photoPre = 1;
  cfg.photoPost = 1;
  cfg.camAddr[0] = 0;
  cfg.camAddrType = 0;
  cfg.camName[0] = 0;
}

static void setDefaults() {
  cfg = Config{};
  cfg.version = CFG_VERSION;
  setPhotoDefaults();

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
  size_t len = prefs.getBytesLength("cfg");
  if (len == sizeof(Config) &&
      prefs.getBytes("cfg", &tmp, sizeof(Config)) == sizeof(Config) &&
      tmp.version == CFG_VERSION) {
    cfg = tmp;
    Serial.println("[cfg] nastavení načteno");
  } else if (len == CFG_V2_SIZE &&
             prefs.getBytes("cfg", &tmp, CFG_V2_SIZE) == CFG_V2_SIZE &&
             tmp.version == CFG_VERSION_V2) {
    // starší verze bez fotografování -> převzít a doplnit výchozí hodnoty
    cfg = tmp;
    cfg.version = CFG_VERSION;
    setPhotoDefaults();
    save();
    Serial.println("[cfg] nastavení převedeno z v2");
  } else {
    setDefaults();
    Serial.println("[cfg] výchozí nastavení");
  }
  cfg.ssid[sizeof(cfg.ssid) - 1] = 0;
  cfg.pass[sizeof(cfg.pass) - 1] = 0;
  cfg.camAddr[sizeof(cfg.camAddr) - 1] = 0;
  cfg.camName[sizeof(cfg.camName) - 1] = 0;
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
  o["photoOn"] = cfg.photoOn;
  o["photoSlow"] = cfg.photoSlow;
  o["photoBack"] = cfg.photoBack;
  o["photoStep"] = cfg.photoStep;
  o["photoSlowSpeed"] = cfg.photoSlowSpeed;
  o["photoPre"] = cfg.photoPre;
  o["photoPost"] = cfg.photoPost;

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
  o["camAddr"] = cfg.camAddr;
  o["camName"] = cfg.camName;
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
  rdB(o, "photoOn", cfg.photoOn);
  rdB(o, "photoSlow", cfg.photoSlow);
  rdB(o, "photoBack", cfg.photoBack);
  rdF(o, "photoStep", cfg.photoStep, 0.1f, 10000);
  rdF(o, "photoSlowSpeed", cfg.photoSlowSpeed, 0.01f, 1000);
  rdF(o, "photoPre", cfg.photoPre, 0, 3600);
  rdF(o, "photoPost", cfg.photoPost, 0, 3600);
}

void Settings::setCamera(const char *addr, uint8_t type, const char *name) {
  strlcpy(cfg.camAddr, addr, sizeof(cfg.camAddr));
  cfg.camAddrType = type;
  strlcpy(cfg.camName, name, sizeof(cfg.camName));
  save();
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
