/*
  TmcUartTest – diagnostika UART mezi ESP32-S3 a TMC2209
  Serial Monitor 115200. Každé 2 s vypíše výsledek.

  1) ECHO test: na jednodrátovém UARTu (PDN) ESP vidí na RX vlastní vysílaná data.
     Žádné echo = přerušené spojení GPIO17 -> rezistor -> PDN -> GPIO18.
  2) Driver test: čtení registrů IOIN (verze má být 0x21), IFCNT, GSTAT.
*/
#include <TMCStepper.h>

#define PIN_TMC_TX  17
#define PIN_TMC_RX  18
#define R_SENSE     0.11f
#define ADDR        0b00

TMC2209Stepper driver(&Serial1, R_SENSE, ADDR);

static void echoTest() {
  while (Serial1.available()) Serial1.read();
  const uint8_t pat[] = {0xA3, 0x3C, 0xC6};
  Serial1.write(pat, sizeof(pat));
  Serial1.flush();
  delay(5);
  uint8_t buf[8];
  int n = 0;
  while (Serial1.available() && n < 8) buf[n++] = Serial1.read();

  Serial.printf("ECHO: odesláno A3 3C C6, přijato %d B:", n);
  for (int i = 0; i < n; i++) Serial.printf(" %02X", buf[i]);
  bool ok = n >= 3 && buf[0] == 0xA3 && buf[1] == 0x3C && buf[2] == 0xC6;
  Serial.println(ok ? "  -> OK (vodiče TX/RX/PDN propojené)"
                    : "  -> CHYBA (přerušený vodič, špatný pin nebo zkrat linky na GND/3V3)");
  delay(10);  // TMC se po neplatných datech zresynchronizuje
}

static void driverTest() {
  uint8_t res = driver.test_connection();
  uint32_t ioin = driver.IOIN();
  uint8_t ver = driver.version();
  Serial.printf("DRIVER: test_connection=%u (0=OK, 1=bez odpovědi/linka v HIGH, 2=linka v LOW)\n", res);
  Serial.printf("        IOIN=0x%08lX  verze=0x%02X (správně 0x21)\n", (unsigned long)ioin, ver);
  if (ver != 0x21) {
    Serial.println("        -> driver neodpovídá: VM napájení? MS1/MS2 na GND? GND společná? prohozené TX/RX?");
    return;
  }
  uint8_t c1 = driver.IFCNT();
  driver.toff(4);                    // zápis -> IFCNT se má zvýšit
  uint8_t c2 = driver.IFCNT();
  Serial.printf("        IFCNT %u -> %u  %s\n", c1, c2, (uint8_t)(c2 - c1) == 1 ? "(zápis OK)" : "(ZÁPIS NEPROŠEL)");
  uint8_t gs = driver.GSTAT();
  Serial.printf("        GSTAT=0x%02X  reset=%u drv_err=%u uv_cp=%u\n", gs, gs & 1, (gs >> 1) & 1, (gs >> 2) & 1);
  Serial.printf("        MS1/MS2 piny (IOIN): MS1=%lu MS2=%lu  ENN=%lu\n",
                (unsigned long)((ioin >> 2) & 1), (unsigned long)((ioin >> 3) & 1), (unsigned long)(ioin & 1));
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== TMC2209 UART test ===");
  Serial.printf("TX=GPIO%d  RX=GPIO%d  adresa=%d\n", PIN_TMC_TX, PIN_TMC_RX, ADDR);
  Serial1.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  driver.begin();
}

void loop() {
  Serial.println("--------------------------------");
  echoTest();
  driverTest();
  delay(2000);
}
