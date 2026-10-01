#pragma once
#include <Arduino.h>

// WiFi (AP + klient současně), mDNS, web server z LittleFS, WebSocket
namespace Net {
  void begin();
  void update();                                   // volat z loop()
  void sendMsg(const char *text, bool isError);    // toast do všech otevřených UI
}
