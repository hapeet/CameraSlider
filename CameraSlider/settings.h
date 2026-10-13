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

  // --- fotografování (BLE spoušť Canon) – přidáno ve v3, musí zůstat na konci ---
  bool photoOn;                 // v automatickém režimu fotit
  bool photoSlow;               // false = v bodě fotky zastavit, true = jen zpomalit
  bool photoBack;               // fotit i při jízdě zpět (Do→Od)
  float photoStep;              // mm  vzdálenost mezi snímky
  float photoSlowSpeed;         // mm/s rychlost průjezdu bodem fotky
  float photoPre;               // s   před snímkem (stání / pomalá jízda)
  float photoPost;              // s   po snímku (stání / pomalá jízda)
  char camAddr[18];             // BLE adresa spárovaného fotoaparátu, "" = žádný
  uint8_t camAddrType;
  char camName[32];
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
  void setCamera(const char *addr, uint8_t type, const char *name);   // "" = zapomenout
}
