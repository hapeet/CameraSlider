#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Všechna nastavení – ukládají se jako jeden blob do NVS (Preferences)
struct Config {
  uint32_t version;

  // --- pohyb (lze měnit i za běhu) ---
  float rangeFrom, rangeTo;     // mm
  float speedOut;               // mm/s  směr od home (+)
  float speedIn;                // mm/s  směr k home (−)
  float waitFar, waitNear;      // s     čekání na vzdálenějším / bližším konci
  float accel;                  // mm/s²
  float smooth;                 // mm    délka náběhu zrychlení (vyhlazení)
  float jogSpeed;               // mm/s
  bool cycleInfinite;
  uint32_t cycleCount;
  bool singleOneWay;            // true = Od→Do, false = Od→Do→Od

  // --- konfigurace (jen v klidu) ---
  float maxPos;                 // mm – fyzický konec pojezdu
  float steps100;               // celých kroků na 100 mm (bez mikrokrokování)
  uint16_t microsteps;
  uint16_t currentMa;           // RMS
  bool stealthChop;
  bool dirInvert;
  float homeSpeed;              // mm/s
  float homeBackoff;            // mm (jen výchozí hodnota, není v UI)
  bool hallActiveLow;

  // --- WiFi klient ---
  char ssid[33];
  char pass[65];
};

extern Config cfg;

namespace Settings {
  void begin();
  void save();
  void factoryReset();                    // smaže NVS a nastaví výchozí hodnoty
  float stepsPerMm();
  void toJson(JsonObject o);
  void applyMotion(JsonObjectConst o);
  void applyConfig(JsonObjectConst o, bool &needRehome);
}
