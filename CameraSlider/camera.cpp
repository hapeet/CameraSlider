#include "camera.h"
#include "config.h"
#include "settings.h"
#include <NimBLEDevice.h>

// ============================================================================
//  Protokol Canon BR-E1 (zjištěno reverzním inženýrstvím, viz ESP32-Canon-BLE-Remote)
//  - služba 00050000-…, zápis jména (0x03 + jméno) do 00050002 = registrace ovladače
//  - zápis 1 bajtu do 00050003 = stav tlačítek
// ============================================================================
static const char *SVC_UUID = "00050000-0000-1000-0000-d8492fffa821";
static const char *PAIR_UUID = "00050002-0000-1000-0000-d8492fffa821";
static const char *SHUTTER_UUID = "00050003-0000-1000-0000-d8492fffa821";

static constexpr uint8_t BTN_RELEASE = 0x80;
static constexpr uint8_t MODE_IMMEDIATE = 0x0C;

enum class CamState : uint8_t { None, Idle, Pairing, Connecting, Connected };

// --- sdíleno mezi taskem a loop() ---
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile CamState state = CamState::None;
static volatile bool reqPair = false, reqForget = false, reqShot = false;
static volatile uint32_t shots = 0;
static char addr[18] = "";            // chráněno mux
static uint8_t addrType = 0;
static char name[32] = "";
static volatile bool pairedEvent = false;   // task -> loop: uložit nové spárování

struct Msg { char text[96]; bool err; };
static QueueHandle_t msgQueue;
static TaskHandle_t taskHandle;
static Camera::Notifier notifier = nullptr;

// --- jen v tasku ---
static NimBLEClient *client = nullptr;
static NimBLERemoteCharacteristic *shutterChr = nullptr;
static uint32_t lastTry = 0;

static void post(bool err, const char *fmt, ...) {
  Msg m;
  m.err = err;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(m.text, sizeof(m.text), fmt, ap);
  va_end(ap);
  Serial.printf("[cam] %s\n", m.text);
  xQueueSend(msgQueue, &m, 0);
}

static void wake() {
  if (taskHandle) xTaskNotifyGive(taskHandle);
}

class ClientCb : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient *, int reason) override {
    shutterChr = nullptr;
    if (state == CamState::Connected) {
      state = addr[0] ? CamState::Idle : CamState::None;
      lastTry = millis();
      post(false, "Camera disconnected");
    }
  }
  // fotoaparát nemá klávesnici -> případné dotazy na PIN rovnou potvrdit
  void onPassKeyEntry(NimBLEConnInfo &ci) override { NimBLEDevice::injectPassKey(ci, 123456); }
  void onConfirmPasskey(NimBLEConnInfo &ci, uint32_t) override { NimBLEDevice::injectConfirmPasskey(ci, true); }
};
static ClientCb clientCb;

static bool writeChr(NimBLERemoteCharacteristic *c, const uint8_t *data, size_t len) {
  return c->writeValue(data, len, !c->canWriteNoResponse());
}

// Po navázání spojení: šifrování, nalezení služby, registrace jména
static bool setupConnection(bool pairing) {
  if (!client->secureConnection())
    Serial.println("[cam] šifrování spojení selhalo, zkouším pokračovat");

  NimBLERemoteService *svc = client->getService(SVC_UUID);
  if (!svc) {
    post(true, "Camera does not offer the remote service – is Bluetooth set to Remote?");
    return false;
  }
  NimBLERemoteCharacteristic *pairChr = svc->getCharacteristic(PAIR_UUID);
  NimBLERemoteCharacteristic *shut = svc->getCharacteristic(SHUTTER_UUID);
  if (!shut) {
    post(true, "Camera shutter characteristic not found");
    return false;
  }

  if (pairChr) {
    uint8_t buf[sizeof(BLE_NAME) + 1];
    buf[0] = 0x03;
    memcpy(buf + 1, BLE_NAME, sizeof(BLE_NAME) - 1);
    buf[sizeof(buf) - 1] = ' ';
    if (!writeChr(pairChr, buf, sizeof(buf)) && pairing) {
      post(true, "Camera rejected pairing");
      return false;
    }
  }
  shutterChr = shut;
  return true;
}

static void disconnect() {
  shutterChr = nullptr;
  if (client->isConnected()) client->disconnect();
  for (int i = 0; i < 50 && client->isConnected(); i++) vTaskDelay(pdMS_TO_TICKS(20));
}

// ============================================================================
static void doPair() {
  disconnect();
  state = CamState::Pairing;
  post(false, "Searching for camera… (camera: Bluetooth function → Remote → Pairing)");

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  NimBLEUUID svcUuid(SVC_UUID);
  uint32_t start = millis();

  while (millis() - start < CAM_PAIR_TIMEOUT_MS && !reqForget) {
    NimBLEScanResults res = scan->getResults(2000, false);
    const NimBLEAdvertisedDevice *found = nullptr;
    for (const NimBLEAdvertisedDevice *d : res) {
      if (d->isAdvertisingService(svcUuid)) { found = d; break; }
    }
    if (!found) continue;

    std::string devName = found->haveName() ? found->getName() : "Canon";
    NimBLEAddress a = found->getAddress();
    Serial.printf("[cam] nalezen %s (%s)\n", devName.c_str(), a.toString().c_str());

    if (!client->connect(found)) {
      post(true, "Cannot connect to %s, retrying…", devName.c_str());
      continue;
    }
    if (!setupConnection(true)) {
      disconnect();
      break;
    }

    portENTER_CRITICAL(&mux);
    strlcpy(addr, a.toString().c_str(), sizeof(addr));
    addrType = a.getType();
    strlcpy(name, devName.c_str(), sizeof(name));
    portEXIT_CRITICAL(&mux);
    pairedEvent = true;
    state = CamState::Connected;
    post(false, "Paired with %s", devName.c_str());
    scan->clearResults();
    return;
  }

  scan->clearResults();
  if (state == CamState::Pairing) {
    state = addr[0] ? CamState::Idle : CamState::None;
    if (!reqForget) post(true, "Camera not found – enable pairing on the camera and try again");
  }
}

static void tryReconnect() {
  char a[18];
  uint8_t t;
  portENTER_CRITICAL(&mux);
  strlcpy(a, addr, sizeof(a));
  t = addrType;
  portEXIT_CRITICAL(&mux);
  if (!a[0]) return;

  state = CamState::Connecting;
  bool ok = client->connect(NimBLEAddress(std::string(a), t)) && setupConnection(false);
  lastTry = millis();
  if (ok) {
    state = CamState::Connected;
    post(false, "Camera connected");
  } else {
    disconnect();
    state = CamState::Idle;
  }
}

static void doShot() {
  if (!shutterChr || !client->isConnected()) return;
  uint8_t press = MODE_IMMEDIATE | BTN_RELEASE;
  uint8_t release = MODE_IMMEDIATE;
  bool ok = writeChr(shutterChr, &press, 1);
  vTaskDelay(pdMS_TO_TICKS(CAM_PRESS_MS));
  ok = writeChr(shutterChr, &release, 1) && ok;
  if (ok) shots = shots + 1;
  else post(true, "Shutter command failed");
}

static void task(void *) {
  NimBLEDevice::init(BLE_NAME);
  NimBLEDevice::setSecurityAuth(true, false, true);    // bonding, bez MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  client = NimBLEDevice::createClient();
  client->setClientCallbacks(&clientCb, false);
  client->setConnectTimeout(CAM_CONNECT_TIMEOUT_MS);
  lastTry = millis() - CAM_RETRY_MS;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

    if (reqForget) {
      reqForget = false;
      reqPair = false;
      disconnect();
      NimBLEDevice::deleteAllBonds();
      portENTER_CRITICAL(&mux);
      addr[0] = 0;
      name[0] = 0;
      portEXIT_CRITICAL(&mux);
      state = CamState::None;
    }
    if (reqPair) {
      reqPair = false;
      doPair();
    }
    if (reqShot) {
      reqShot = false;
      doShot();
    }
    if (state == CamState::Connected && !client->isConnected()) {
      shutterChr = nullptr;
      state = CamState::Idle;
    }
    if (state == CamState::Idle && millis() - lastTry >= CAM_RETRY_MS) tryReconnect();
  }
}

// ============================================================================
void Camera::begin() {
  msgQueue = xQueueCreate(8, sizeof(Msg));
  strlcpy(addr, cfg.camAddr, sizeof(addr));
  addrType = cfg.camAddrType;
  strlcpy(name, cfg.camName, sizeof(name));
  state = addr[0] ? CamState::Idle : CamState::None;
  xTaskCreatePinnedToCore(task, "camera", 6144, nullptr, 2, &taskHandle, 0);
}

void Camera::setNotifier(Notifier n) { notifier = n; }

void Camera::update() {
  Msg m;
  while (xQueueReceive(msgQueue, &m, 0) == pdTRUE)
    if (notifier) notifier(m.text, m.err);

  if (pairedEvent) {
    pairedEvent = false;
    char a[18], n[32];
    uint8_t t;
    portENTER_CRITICAL(&mux);
    strlcpy(a, addr, sizeof(a));
    strlcpy(n, name, sizeof(n));
    t = addrType;
    portEXIT_CRITICAL(&mux);
    Settings::setCamera(a, t, n);
  }
}

const char *Camera::cmdPair() {
  if (state == CamState::Pairing) return "Pairing already in progress";
  reqPair = true;
  wake();
  return nullptr;
}

void Camera::cmdForget() {
  reqForget = true;
  wake();
  Settings::setCamera("", 0, "");
}

const char *Camera::cmdTestShot() {
  if (!shoot()) return "Camera not connected";
  return nullptr;
}

bool Camera::shoot() {
  if (state != CamState::Connected) return false;
  reqShot = true;
  wake();
  return true;
}

bool Camera::isConnected() { return state == CamState::Connected; }

void Camera::fillStatus(JsonObject o) {
  static const char *const names[] = { "none", "idle", "pairing", "connecting", "connected" };
  o["st"] = names[(uint8_t)state];
  o["shots"] = shots;
  char n[32];
  portENTER_CRITICAL(&mux);
  strlcpy(n, name, sizeof(n));
  portEXIT_CRITICAL(&mux);
  o["name"] = n;
}
