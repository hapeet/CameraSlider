#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Bluetooth LE dálková spoušť pro Canon (protokol BR-E1: 77D, 200D, 6D II, M50, R...)
// Veškerá komunikace s BLE běží ve vlastním tasku – připojování je blokující
// a nesmí brzdit pohyb.
namespace Camera {
  typedef void (*Notifier)(const char *msg, bool isError);

  void begin();
  void update();                 // volat z loop() – předává zprávy a ukládá spárování
  void setNotifier(Notifier n);

  const char *cmdPair();         // hledat fotoaparát v režimu párování
  void cmdForget();              // zapomenout spárovaný fotoaparát
  const char *cmdTestShot();

  bool shoot();                  // false = fotoaparát není připojen
  bool isConnected();
  void fillStatus(JsonObject o);
}
