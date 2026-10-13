#include "motion.h"
#include "config.h"
#include "settings.h"
#include "camera.h"
#include <FastAccelStepper.h>
#include <TMCStepper.h>

static FastAccelStepperEngine engine;
static FastAccelStepper *stepper = nullptr;
static TMC2209Stepper driver(&Serial1, TMC_R_SENSE, TMC_ADDRESS);

static MotionState state = MotionState::Idle;
static bool homed = false;
static bool enabled = false;
static bool driverOk = false;
static uint32_t lastDriverCheck = 0;
static Motion::Notifier notifier = nullptr;

// --- homing ---
enum class HomePhase : uint8_t { Leave, Seek, SeekStop, Backoff, Fine, FineStop, ToZero };
static HomePhase hPhase;
static int32_t hTrigger;

// --- program (cyklus / 1 cyklus) ---
enum class ProgPhase : uint8_t { ToStart, ToB, WaitB, ToA, WaitA };
static ProgPhase pPhase;
static bool pCycleMode, pOneWay;
static uint32_t pDone, pTarget;   // pTarget 0 = nekonečně
static float pA, pB;              // Od, Do (zafixováno při startu)
static uint32_t waitStart, waitMs;

// --- fotografování během jízdy Od↔Do ---
// Body snímků: pA + k·krok (k = 0..shotN), stejné pro jízdu tam i zpět.
enum class ShotPhase : uint8_t { Move, Pre, Post };
static bool pPhoto, pSlow;        // zafixováno při startu
static float pStep;
static int32_t shotN;
static bool legPhoto;             // aktuální jízda fotí
static float legS, legE;          // začátek / konec jízdy
static int legDir;                // +1 / −1 (směr osy)
static int32_t shotK, shotDK;     // příští bod a směr indexu
static ShotPhase sPhase;
static uint32_t sStart, sMs;
static float lastShotPos;         // poslední bod snímku (NAN = žádný)
static float postEndU;            // konec pomalého úseku za posledním snímkem
static bool slowNow;              // právě je nastavena pomalá rychlost
static uint32_t shotsDone, shotsMissed;

// --- ruční posuv ---
static int jogDir = 0;
static uint32_t jogKeepalive = 0;

// ============================================================================
static void emit(const char *msg, bool err) {
  Serial.printf("[motion] %s\n", msg);
  if (notifier) notifier(msg, err);
}

static float spm() { return Settings::stepsPerMm(); }
static int32_t toSteps(float mm) { return lroundf(mm * spm()); }
static float toMm(int32_t st) { return st / spm(); }
static float clampPos(float mm) { return constrain(mm, 0.0f, cfg.maxPos); }
static bool running() { return stepper->isRunning(); }
static float curMm() { return toMm(stepper->getCurrentPosition()); }

static void setProfile(float speedMm, float accelMm, float smoothMm) {
  uint32_t hz = (uint32_t)constrain(speedMm * spm(), 1.0f, (float)MAX_STEP_HZ);
  stepper->setSpeedInHz(hz);
  stepper->setAcceleration((int32_t)max(1.0f, accelMm * spm()));
  stepper->setLinearAcceleration((uint32_t)(smoothMm * spm()));
}

static void moveToMm(float mm, float speed) {
  setProfile(speed, cfg.accel, cfg.smooth);
  stepper->moveTo(toSteps(clampPos(mm)));
}

// rychlost podle směru: ven (od home) / zpět (k home)
static void startLeg(float mm) {
  moveToMm(mm, mm >= curMm() ? cfg.speedOut : cfg.speedIn);
}

static const char *needReady() {
  if (!stepper) return "Motor not initialized";
  if (!homed) return "Run homing first";
  return nullptr;
}

// ============================================================================
void Motion::begin() {
  pinMode(PIN_HALL, INPUT_PULLUP);
  Serial1.begin(TMC_BAUD, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  delay(50);   // linka v klidu -> TMC zahodí případné smetí z doby resetu

  engine.init();
  stepper = engine.stepperConnectToPin(PIN_STEP);
  if (!stepper) {
    emit("Cannot initialize STEP pin", true);
    return;
  }
  stepper->setDirectionPin(PIN_DIR);
  stepper->setEnablePin(PIN_EN);        // aktivní v LOW
  stepper->setAutoEnable(false);
  stepper->disableOutputs();

  applyDriverSettings();
}

void Motion::setNotifier(Notifier n) { notifier = n; }

static uint32_t drvFailTotal = 0;   // statistika chyb UART (diagnostika)
static uint8_t drvFailRow = 0;

// test_connection s opakováním – jedno zkreslené čtení není chyba
static bool driverResponds() {
  for (int i = 0; i < 3; i++) {
    if (driver.test_connection() == 0) return true;
    drvFailTotal++;
    delay(5);
  }
  return false;
}

void Motion::applyDriverSettings() {
  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.I_scale_analog(false);
  driver.internal_Rsense(false);
  driver.mstep_reg_select(true);   // mikrokroky z registru, ne z pinů MS1/MS2
  driver.pdn_disable(true);        // PDN pin = UART
  driver.rms_current(cfg.currentMa);
  driver.microsteps(cfg.microsteps);
  driver.intpol(true);
  driver.en_spreadCycle(!cfg.stealthChop);
  driver.pwm_autoscale(true);
  driver.shaft(cfg.dirInvert);
  driver.GSTAT(0b111);             // smazat příznak resetu

  // ověření zpětným čtením – ověří i to, že zápisy prošly
  uint16_t msRead = driver.microsteps();           // TMCStepper vrací 0 pro celé kroky
  driverOk = driverResponds() && (msRead ? msRead : 1) == cfg.microsteps;
  drvFailRow = 0;
  if (!driverOk) emit("TMC2209 not responding over UART – check PDN_UART, TX/RX and VIO supply", true);
  else Serial.printf("[motion] TMC2209 OK, %u mA, 1/%u\n", cfg.currentMa, cfg.microsteps);
}

// Průběžná kontrola driveru: naskočí-li VM později nebo driver projde resetem
// (výpadek VM), znovu se nakonfiguruje. Výpadek během provozu ruší homing.
static void checkDriver() {
  bool ok = driver.test_connection() == 0;
  if (!ok) {
    drvFailTotal++;
    Serial.printf("[tmc] čtení selhalo (celkem %lu)\n", (unsigned long)drvFailTotal);
    if (driverOk && ++drvFailRow < 3) return;   // výpadek až po 3 neúspěších za sebou
  } else {
    drvFailRow = 0;
  }

  if (ok && !driverOk) {
    Motion::applyDriverSettings();
    if (driverOk) emit("TMC2209 connected", false);
  } else if (ok && (driver.GSTAT() & 0x01)) {
    Motion::applyDriverSettings();
    if (homed) {
      homed = false;
      emit("TMC2209 was reset (VM power loss?) – homing required", true);
    }
  } else if (!ok && driverOk) {
    driverOk = false;
    emit("TMC2209 lost – check VM supply and UART wiring", true);
  }
}

void Motion::invalidateHome() {
  homed = false;
}

// ============================================================================
//  Homing
//  1) je-li čidlo aktivní, odjede ven
//  2) hledá čidlo směrem k 0, zapamatuje si krok sepnutí, plynule zabrzdí
//  3) souřadnice posune tak, aby bod sepnutí = 0
//  4) odjede o backoff; pokud čidlo pustilo, zopakuje hledání pomalu (přesnost)
//     (u západkového čidla SS41F nemusí pustit -> jemné hledání se přeskočí)
//  5) najede na 0
// ============================================================================
static void homeFail(const char *msg) {
  stepper->forceStop();
  state = MotionState::Idle;
  emit(msg, true);
}

static void startSeek() {
  hPhase = HomePhase::Seek;
  setProfile(cfg.homeSpeed, HOME_ACCEL_MMS2, 0);
  stepper->moveTo(stepper->getCurrentPosition() - toSteps(cfg.maxPos + 50));
}

const char *Motion::cmdHome() {
  if (!stepper) return "Motor not initialized";
  if (state != MotionState::Idle) return "Stop the motion first";
  if (!driverOk) applyDriverSettings();   // zkusit znovu

  stepper->enableOutputs();
  enabled = true;
  homed = false;
  stepper->setCurrentPosition(0);
  state = MotionState::Homing;

  if (hallActive()) {
    hPhase = HomePhase::Leave;
    setProfile(cfg.homeSpeed, HOME_ACCEL_MMS2, 0);
    stepper->moveTo(toSteps(cfg.homeBackoff * 2));
  } else {
    startSeek();
  }
  emit("Homing…", false);
  return nullptr;
}

static void updateHoming() {
  switch (hPhase) {
    case HomePhase::Leave:
      if (!running()) {
        if (Motion::hallActive()) return homeFail("Sensor still active after moving away – check magnet / sensor type");
        startSeek();
      }
      break;

    case HomePhase::Seek:
      if (Motion::hallActive()) {
        hTrigger = stepper->getCurrentPosition();
        stepper->stopMove();
        hPhase = HomePhase::SeekStop;
      } else if (!running()) {
        homeFail("Home sensor not found");
      }
      break;

    case HomePhase::SeekStop:
      if (!running()) {
        stepper->setCurrentPosition(stepper->getCurrentPosition() - hTrigger);
        hPhase = HomePhase::Backoff;
        setProfile(cfg.homeSpeed, HOME_ACCEL_MMS2, 0);
        stepper->moveTo(toSteps(cfg.homeBackoff));
      }
      break;

    case HomePhase::Backoff:
      if (!running()) {
        if (Motion::hallActive()) {
          // západkové čidlo nepustilo -> bereme hrubý výsledek
          hPhase = HomePhase::ToZero;
          setProfile(cfg.homeSpeed, HOME_ACCEL_MMS2, 0);
          stepper->moveTo(0);
        } else {
          hPhase = HomePhase::Fine;
          setProfile(max(cfg.homeSpeed * 0.2f, 0.5f), HOME_ACCEL_MMS2, 0);
          stepper->moveTo(-toSteps(cfg.homeBackoff));
        }
      }
      break;

    case HomePhase::Fine:
      if (Motion::hallActive()) {
        hTrigger = stepper->getCurrentPosition();
        stepper->stopMove();
        hPhase = HomePhase::FineStop;
      } else if (!running()) {
        homeFail("Home sensor not found during fine approach");
      }
      break;

    case HomePhase::FineStop:
      if (!running()) {
        stepper->setCurrentPosition(stepper->getCurrentPosition() - hTrigger);
        hPhase = HomePhase::ToZero;
        setProfile(cfg.homeSpeed, HOME_ACCEL_MMS2, 0);
        stepper->moveTo(0);
      }
      break;

    case HomePhase::ToZero:
      if (!running()) {
        homed = true;
        state = MotionState::Idle;
        emit("Homing complete", false);
      }
      break;
  }
}

// ============================================================================
//  Manuální polohování
// ============================================================================
const char *Motion::cmdGoto(float mm) {
  if (auto e = needReady()) return e;
  if (state != MotionState::Idle && state != MotionState::Moving) return "Another motion in progress – press Stop first";
  state = MotionState::Moving;
  startLeg(mm);
  return nullptr;
}

const char *Motion::cmdJogStep(float d) {
  if (auto e = needReady()) return e;
  if (state != MotionState::Idle && state != MotionState::Moving) return "Another motion in progress – press Stop first";
  float base = (state == MotionState::Moving) ? toMm(stepper->targetPos()) : curMm();
  state = MotionState::Moving;
  moveToMm(base + d, cfg.jogSpeed);
  return nullptr;
}

const char *Motion::cmdJogHold(int dir) {
  if (auto e = needReady()) return e;
  dir = dir > 0 ? 1 : -1;
  if (state == MotionState::Jogging && dir == jogDir) {
    jogKeepalive = millis();
    return nullptr;
  }
  if (state != MotionState::Idle && state != MotionState::Moving && state != MotionState::Jogging)
    return "Another motion in progress – press Stop first";
  jogDir = dir;
  jogKeepalive = millis();
  state = MotionState::Jogging;
  moveToMm(dir > 0 ? cfg.maxPos : 0, cfg.jogSpeed);
  return nullptr;
}

void Motion::cmdJogRelease() {
  if (state == MotionState::Jogging && jogDir) {
    stepper->stopMove();
    jogDir = 0;
  }
}

// ============================================================================
//  Fotografování během jízdy
//  Zastavit: najet na bod -> čekání „před“ -> snímek -> čekání „po“ -> další bod
//  Zpomalit: jízda plnou rychlostí, před bodem zpomalení tak, aby vozík jel
//            pomalou rychlostí „před“ s před bodem a „po“ s za ním; snímek se
//            spustí při průjezdu bodem.
// ============================================================================
// tolerance polohy bodu: alespoň ~půl kroku (při hrubém mikrokrokování je krok velký)
static float shotEps() { return max(0.02f, 0.6f / spm()); }

static float shotPos(int32_t k) { return pA + k * pStep * (pB >= pA ? 1 : -1); }
static bool hasShot() { return shotK >= 0 && shotK <= shotN; }
static float legU(float mm) { return (mm - legS) * legDir; }   // ujetá dráha od začátku jízdy
static float cruiseSpeed() { return legDir > 0 ? cfg.speedOut : cfg.speedIn; }
static float slowSpeed() { return min(cfg.photoSlowSpeed, cruiseSpeed()); }

static void fireShot(float at) {
  lastShotPos = at;
  shotsDone++;
  if (!Camera::shoot()) {
    if (!shotsMissed) emit("Camera not connected – shot skipped", true);
    shotsMissed++;
  }
}

static void startShotWait(ShotPhase ph, float s) {
  sPhase = ph;
  sMs = (uint32_t)(s * 1000.0f);
  sStart = millis();
}

// otočka na konci dráhy: bod na startu jízdy už byl vyfocen -> nefotit dvakrát
static void skipDoneShot() {
  if (!hasShot() || isnan(lastShotPos)) return;
  float p = shotPos(shotK);
  if (fabsf(p - legS) < shotEps() && fabsf(p - lastShotPos) < shotEps()) shotK += shotDK;
}

// --- režim Zastavit ---
static bool stopNext() {   // false = jízda dokončena
  skipDoneShot();
  if (hasShot()) {
    float p = shotPos(shotK);
    if (fabsf(curMm() - p) < shotEps()) startShotWait(ShotPhase::Pre, cfg.photoPre);
    else { sPhase = ShotPhase::Move; startLeg(p); }
    return true;
  }
  if (fabsf(curMm() - legE) >= shotEps()) { sPhase = ShotPhase::Move; startLeg(legE); return true; }
  return false;
}

// --- režim Zpomalit ---
static void setLegSpeed(bool slow) {
  if (slow == slowNow) return;
  slowNow = slow;
  uint32_t hz = (uint32_t)constrain((slow ? slowSpeed() : cruiseSpeed()) * spm(), 1.0f, (float)MAX_STEP_HZ);
  stepper->setSpeedInHz(hz);
  stepper->applySpeedAcceleration();
}

static void slowStartMove() {
  slowNow = !isnan(postEndU) && postEndU > 0;   // za snímkem na startu se jede pomalu
  sPhase = ShotPhase::Move;
  moveToMm(legE, slowNow ? slowSpeed() : cruiseSpeed());
}

static void slowRegulate() {
  float u = legU(curMm());
  float vs = slowSpeed();
  bool slow = !isnan(postEndU) && u < postEndU;
  if (!slow && hasShot()) {
    float zone = legU(shotPos(shotK)) - vs * cfg.photoPre;   // začátek pomalého úseku
    float v = fabsf(stepper->getCurrentSpeedInMilliHz() / 1000.0f / spm());
    float brake = max(0.0f, v * v - vs * vs) / (2.0f * cfg.accel);
    float margin = cfg.smooth + v * 0.03f + 0.2f;   // vyhlazení rampy + zpoždění smyčky
    slow = u >= zone - brake - margin;
  }
  setLegSpeed(slow);
}

// Začátek jízdy S -> E. false = jízda bez pohybu dokončena
static bool startPhotoLeg(bool toB) {
  legS = toB ? pA : pB;
  legE = toB ? pB : pA;
  legDir = legE >= legS ? 1 : -1;
  shotK = toB ? 0 : shotN;
  shotDK = toB ? 1 : -1;
  postEndU = NAN;
  skipDoneShot();

  if (!pSlow) return stopNext();
  if (hasShot() && fabsf(shotPos(shotK) - legS) < shotEps()) {
    startShotWait(ShotPhase::Pre, cfg.photoPre);   // snímek na startu – ve stoje
  } else {
    slowStartMove();
  }
  return true;
}

// true = jízda dokončena
static bool updatePhotoLeg() {
  if (!pSlow) {
    switch (sPhase) {
      case ShotPhase::Move:
        if (running()) return false;
        if (hasShot() && fabsf(curMm() - shotPos(shotK)) < shotEps()) {
          startShotWait(ShotPhase::Pre, cfg.photoPre);
          return false;
        }
        return !stopNext();
      case ShotPhase::Pre:
        if (millis() - sStart < sMs) return false;
        fireShot(shotPos(shotK));
        startShotWait(ShotPhase::Post, max(cfg.photoPost, (CAM_PRESS_MS + 50) / 1000.0f));
        return false;
      case ShotPhase::Post:
        if (millis() - sStart < sMs) return false;
        shotK += shotDK;
        return !stopNext();
    }
    return true;
  }

  switch (sPhase) {
    case ShotPhase::Pre:   // snímek na startu jízdy
      if (millis() - sStart < sMs) return false;
      fireShot(shotPos(shotK));
      postEndU = legU(shotPos(shotK)) + slowSpeed() * cfg.photoPost;
      shotK += shotDK;
      slowStartMove();
      return false;

    case ShotPhase::Move:
      if (hasShot() && legU(curMm()) >= legU(shotPos(shotK)) - shotEps()) {
        float p = shotPos(shotK);
        fireShot(p);
        postEndU = legU(p) + slowSpeed() * cfg.photoPost;
        shotK += shotDK;
      }
      if (running()) {
        slowRegulate();
        return false;
      }
      // dojeto na konec – byl-li tam snímek, počkat dobu „po“
      if (!isnan(lastShotPos) && fabsf(lastShotPos - legE) < shotEps() && cfg.photoPost > 0) {
        startShotWait(ShotPhase::Post, cfg.photoPost);
        return false;
      }
      return true;

    case ShotPhase::Post:
      return millis() - sStart >= sMs;
  }
  return true;
}

// ============================================================================
//  Program: cyklický režim / 1 cyklus
//  Najetí na Od -> Od→Do -> čekání -> Do→Od -> čekání -> ...
// ============================================================================
const char *Motion::cmdStartProgram(bool cycle) {
  if (auto e = needReady()) return e;
  if (state != MotionState::Idle) return "Motion in progress – press Stop first";
  pA = clampPos(cfg.rangeFrom);
  pB = clampPos(cfg.rangeTo);
  if (fabsf(pA - pB) < 0.1f) return "From–To range is zero";

  pPhoto = cfg.photoOn;
  if (pPhoto) {
    if (!Camera::isConnected()) return "Camera not connected – pair it in the Camera section or turn photos off";
    pSlow = cfg.photoSlow;
    pStep = max(cfg.photoStep, 0.1f);
    shotN = (int32_t)floorf(fabsf(pB - pA) / pStep + 0.001f);
  }
  lastShotPos = NAN;
  shotsDone = shotsMissed = 0;

  pCycleMode = cycle;
  pOneWay = !cycle && cfg.singleOneWay;
  pTarget = cycle ? (cfg.cycleInfinite ? 0 : cfg.cycleCount) : 1;
  pDone = 0;
  state = MotionState::Program;
  pPhase = ProgPhase::ToStart;
  startLeg(pA);
  emit(cycle ? "Cycling started" : "Single cycle started", false);
  return nullptr;
}

static void startWait(bool atB) {
  float here = atB ? pB : pA, other = atB ? pA : pB;
  float s = (here >= other) ? cfg.waitFar : cfg.waitNear;
  waitMs = (uint32_t)(s * 1000.0f);
  waitStart = millis();
  pPhase = atB ? ProgPhase::WaitB : ProgPhase::WaitA;
}

static void finishProgram() {
  state = MotionState::Idle;
  if (pPhoto && shotsMissed) {
    char m[112];
    snprintf(m, sizeof(m), "Program finished – %lu of %lu shots skipped (camera not connected)",
             (unsigned long)shotsMissed, (unsigned long)shotsDone);
    emit(m, true);
  } else {
    emit("Program finished", false);
  }
}

// rozjezd jedné jízdy programu (Od→Do / Do→Od), s fotografováním nebo bez
static bool beginLeg(bool toB) {   // false = není kam jet
  legPhoto = pPhoto && (toB || cfg.photoBack);
  if (legPhoto) return startPhotoLeg(toB);
  startLeg(toB ? pB : pA);
  return true;
}

static bool legDone() {
  return legPhoto ? updatePhotoLeg() : !running();
}

static void endLegB() {
  if (pOneWay) finishProgram();
  else startWait(true);
}

static void endLegA() {
  pDone++;
  if (pTarget && pDone >= pTarget) finishProgram();
  else startWait(false);
}

static void updateProgram() {
  switch (pPhase) {
    case ProgPhase::ToStart:
      if (!running()) {
        pPhase = ProgPhase::ToB;
        if (!beginLeg(true)) endLegB();
      }
      break;
    case ProgPhase::ToB:
      if (legDone()) endLegB();
      break;
    case ProgPhase::WaitB:
      if (millis() - waitStart >= waitMs) {
        pPhase = ProgPhase::ToA;
        if (!beginLeg(false)) endLegA();
      }
      break;
    case ProgPhase::ToA:
      if (legDone()) endLegA();
      break;
    case ProgPhase::WaitA:
      if (millis() - waitStart >= waitMs) {
        pPhase = ProgPhase::ToB;
        if (!beginLeg(true)) endLegB();
      }
      break;
  }
}

// ============================================================================
void Motion::cmdStop() {
  if (!stepper || state == MotionState::Idle) return;
  stepper->stopMove();           // plynulé zabrzdění
  jogDir = 0;
  state = MotionState::Moving;   // -> Idle až po zastavení
  emit("Stop", false);
}

void Motion::cmdDisable() {
  if (!stepper) return;
  stepper->forceStop();
  stepper->disableOutputs();
  enabled = false;
  homed = false;
  jogDir = 0;
  state = MotionState::Idle;
  emit("Motor released – homing required", false);
}

void Motion::update() {
  if (!stepper) return;

  // UART čtení může blokovat pár ms -> ne během homingu (čtení čidla)
  if (state != MotionState::Homing && millis() - lastDriverCheck > 1000) {
    lastDriverCheck = millis();
    checkDriver();
  }

  switch (state) {
    case MotionState::Idle: break;
    case MotionState::Homing: updateHoming(); break;
    case MotionState::Moving:
      if (!running()) state = MotionState::Idle;
      break;
    case MotionState::Jogging:
      if (jogDir && millis() - jogKeepalive > JOG_TIMEOUT_MS) {
        stepper->stopMove();
        jogDir = 0;
      }
      if (!running()) { state = MotionState::Idle; jogDir = 0; }
      break;
    case MotionState::Program: updateProgram(); break;
  }
}

bool Motion::isBusy() { return state != MotionState::Idle; }
bool Motion::isHomed() { return homed; }

bool Motion::hallActive() {
  bool lvl = digitalRead(PIN_HALL);
  return cfg.hallActiveLow ? !lvl : lvl;
}

static const char *stateName() {
  switch (state) {
    case MotionState::Idle: return "Idle";
    case MotionState::Homing: return "Homing";
    case MotionState::Moving: return "Moving";
    case MotionState::Jogging: return "Jogging";
    case MotionState::Program: return "Program";
  }
  return "?";
}

static const char *phaseName() {
  switch (pPhase) {
    case ProgPhase::ToStart: return "ToStart";
    case ProgPhase::ToB: return "ToB";
    case ProgPhase::WaitB: return "WaitB";
    case ProgPhase::ToA: return "ToA";
    case ProgPhase::WaitA: return "WaitA";
  }
  return "";
}

static float r2(float v) { return roundf(v * 100.0f) / 100.0f; }

void Motion::fillStatus(JsonObject o) {
  o["state"] = stateName();
  o["homed"] = homed;
  o["enabled"] = enabled;
  o["hall"] = hallActive();
  o["driver"] = driverOk;
  o["drvFails"] = drvFailTotal;
  if (!stepper) return;

  o["pos"] = r2(curMm());
  o["target"] = r2(toMm(stepper->targetPos()));
  o["speed"] = r2(stepper->getCurrentSpeedInMilliHz() / 1000.0f / spm());

  if (state == MotionState::Program) {
    o["phase"] = phaseName();
    o["mode"] = pCycleMode ? "cycle" : "single";
    o["done"] = pDone;
    o["total"] = pTarget;
    if (pPhase == ProgPhase::WaitA || pPhase == ProgPhase::WaitB) {
      uint32_t el = millis() - waitStart;
      o["waitLeft"] = r2(el < waitMs ? (waitMs - el) / 1000.0f : 0);
    }
    if (pPhoto) {
      o["shots"] = shotsDone;
      o["shotsMissed"] = shotsMissed;
      o["shotsPerPass"] = shotN + 1;
      bool legRun = pPhase == ProgPhase::ToB || pPhase == ProgPhase::ToA;
      if (legRun && legPhoto && sPhase != ShotPhase::Move) {
        uint32_t el = millis() - sStart;
        o["shotWait"] = sPhase == ShotPhase::Pre ? "pre" : "post";
        o["waitLeft"] = r2(el < sMs ? (sMs - el) / 1000.0f : 0);
      }
    }
  }
}
