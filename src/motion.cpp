#include "motion.h"

#include "config.h"
#include "hardware.h"

namespace motion {

namespace {
HardwareTimer* tickTimer = nullptr;
Profile fwd;
Profile rot;
volatile bool enabled = false;
volatile bool steering = false;
volatile bool crashFlag = false;
float fwdError = 0, rotError = 0, lastFwdError = 0, lastRotError = 0;
float lastCte = 0;

// ---- Duong cong slalom: goc quay dat theo QUANG DUONG da di (khong theo thoi gian) ----
mm::TurnParams curve;
volatile bool curveActive = false;
float curveS = 0;        // mm da di trong duong cong
float curveDir = 1;      // +1 trai, -1 phai
float curveOmega = 0;    // deg/s (cho feed-forward)
float curveOmegaPrev = 0;

// Volt can thiet de banh xe chay van toc v (mm/s) gia toc a (mm/s^2)
float feedForward(float v, float a) {
  float volts = SPEED_FF * v + ACC_FF * a;
  if (v > 1.0f)
    volts += BIAS_FF;
  else if (v < -1.0f)
    volts -= BIAS_FF;
  return volts;
}

void controlTick() {
  encoders::update();
  sensors::update();
  fwd.update(LOOP_INTERVAL);
  rot.update(LOOP_INTERVAL);

  float rotDelta = rot.delta();
  float rotSpeed = rot.speed();
  float rotAcc = rot.acceleration();
  if (curveActive) {
    const float ds = fwd.delta();
    const float k = mm::curveShape(curve, curveS + 0.5f * ds) / curve.radius;  // rad/mm
    curveS += ds;
    rotDelta = curveDir * k * ds * (180.0f / PI);
    curveOmega = rotDelta / LOOP_INTERVAL;
    rotSpeed = curveOmega;
    rotAcc = (curveOmega - curveOmegaPrev) / LOOP_INTERVAL;
    curveOmegaPrev = curveOmega;
    if (curveS >= curve.length()) {
      curveActive = false;
      fwd.shiftTarget(curve.length());  // moc tiep theo tinh tu cuoi duong cong
      curveOmega = curveOmegaPrev = 0;
    }
  }

  if (!enabled) {
    motors::coast();
    return;
  }

  // ---- Bam tam duong bang cam bien ben ----
  float steer = 0;  // deg/s
  if (steering && fwd.speed() > 50.0f) {
    const float cte = sensors::crossTrackError();
    steer = -(STEERING_KP * cte + STEERING_KD * (cte - lastCte));
    steer = constrain(steer, -STEERING_MAX, STEERING_MAX);
    lastCte = cte;
  } else {
    lastCte = 0;
  }

  // ---- PD vi tri ----
  fwdError += fwd.delta() - encoders::fwdChange;
  rotError += rotDelta - encoders::rotChange + steer * LOOP_INTERVAL;
  const float fwdOut = FWD_KP * fwdError + FWD_KD * (fwdError - lastFwdError);
  const float rotOut = ROT_KP * rotError + ROT_KD * (rotError - lastRotError);
  lastFwdError = fwdError;
  lastRotError = rotError;

  if (fabsf(fwdError) > FWD_ERROR_LIMIT || fabsf(rotError) > ROT_ERROR_LIMIT) {
    crashFlag = true;
    enabled = false;
    motors::coast();
    return;
  }

  // ---- Feed-forward theo van toc tung banh ----
  const float v = fwd.speed();
  const float a = fwd.acceleration();
  const float tangential = rotSpeed * (PI / 180.0f) * (WHEEL_SEPARATION * 0.5f);
  const float tangentialAcc = rotAcc * (PI / 180.0f) * (WHEEL_SEPARATION * 0.5f);
  const float ffLeft = feedForward(v - tangential, a - tangentialAcc);
  const float ffRight = feedForward(v + tangential, a + tangentialAcc);

  motors::setVolts(fwdOut - rotOut + ffLeft, fwdOut + rotOut + ffRight, sensors::batteryVolts);
}

}  // namespace

void begin() {
  fwd.reset();
  rot.reset();
  tickTimer = new HardwareTimer(TIM2);
  tickTimer->setOverflow(LOOP_FREQUENCY, HERTZ_FORMAT);
  tickTimer->attachInterrupt(controlTick);
  tickTimer->resume();
}

void enable() {
  noInterrupts();
  fwdError = rotError = lastFwdError = lastRotError = 0;
  crashFlag = false;
  enabled = true;
  interrupts();
}

void disable() {
  noInterrupts();
  enabled = false;
  steering = false;
  interrupts();
  motors::coast();
}

bool isEnabled() { return enabled; }

void resetProfiles() {
  noInterrupts();
  fwd.reset();
  rot.reset();
  curveActive = false;
  fwdError = rotError = lastFwdError = lastRotError = 0;
  interrupts();
}

void extendForward(float distance, float topSpeed, float finalSpeed, float accel) {
  noInterrupts();
  fwd.extend(distance, topSpeed, finalSpeed, accel);
  interrupts();
}

void startTurn(float angle, float omega, float alpha) {
  noInterrupts();
  rot.reset();
  rot.extend(angle, omega, 0, alpha);
  interrupts();
}

void rebaseForward() {
  noInterrupts();
  fwd.rebase();
  interrupts();
}

void setSteering(bool on) { steering = on; }

void setManualSpeeds(float v, float omega) {
  noInterrupts();
  fwd.setSpeed(v);
  rot.setSpeed(omega);
  interrupts();
}

void stopManual() {
  noInterrupts();
  fwd.stop();
  rot.stop();
  interrupts();
}

void startCurve(const mm::TurnParams& p, int8_t dir) {
  noInterrupts();
  curve = p;
  curveDir = dir > 0 ? 1.0f : -1.0f;
  // Neu robot da troi qua diem bat dau (main cham vai ms) thi tinh luon phan do
  curveS = fwd.position() - fwd.target();
  if (curveS < 0) curveS = 0;
  curveOmegaPrev = 0;
  curveActive = true;
  interrupts();
}

bool curveDone() { return !curveActive; }

bool forwardDone() { return fwd.isFinished(); }
bool turnDone() { return rot.isFinished(); }
float forwardPosition() {
  noInterrupts();
  float p = fwd.position();
  interrupts();
  return p;
}
float forwardSpeed() { return fwd.speed(); }

bool crashed() { return crashFlag; }
void clearCrash() { crashFlag = false; }

}  // namespace motion
