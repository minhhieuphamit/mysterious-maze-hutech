#include "robot.h"

#include "hardware.h"
#include "motion.h"

namespace robot {

using mm::Turn;

// ----------------------------------------------------------------- cau hinh re
static mm::RunConfig gRun[RUN_PARAM_COUNT];
static mm::TurnParams gSearchTurn;
static bool gSmoothSearch = false;

bool initTurns() {
  bool ok = true;
  for (uint8_t i = 0; i < RUN_PARAM_COUNT; i++) {
    mm::RunConfig& c = gRun[i];
    const RunParams& p = RUN_PARAMS[i];
    c.halfCell = HALF_CELL;
    c.startToCenter = START_TO_CENTER;
    c.topSpeed = p.topSpeed;
    c.diagSpeed = p.diagSpeed;
    c.accel = p.accel;
    c.lateralAccel = p.lateralAccel;
    c.maxTurnSpeed = p.maxTurnSpeed;
    ok &= c.init(TURN_SPECS);
  }
  gSearchTurn = gRun[0].turn[mm::TURN_SS90];
  // Re cong khi tham do can: doan thang truoc duong cong >= diem doc cam bien + 3 mm du tru
  gSmoothSearch = SEARCH_PARAMS.smoothTurns && gSearchTurn.valid && gSearchTurn.before >= SENSING_OFFSET + 3.0f;
  return ok;
}

const mm::RunConfig& runConfig(uint8_t i) { return gRun[i < RUN_PARAM_COUNT ? i : 0]; }
const mm::TurnParams& searchTurn() { return gSearchTurn; }
bool smoothSearchAvailable() { return gSmoothSearch; }

// ----------------------------------------------------------------- tien ich
static bool aborted() {
  if (motion::crashed()) return true;
  if (ui::goPressed()) {  // nhan GO khi dang chay = dung khan
    motion::disable();
    return true;
  }
  return false;
}

template <typename F>
static bool waitFor(F done) {
  while (!done()) {
    if (aborted()) return false;
    delay(1);
  }
  return true;
}

bool waitForward() { return waitFor(motion::forwardDone); }
bool waitTurn() { return waitFor(motion::turnDone); }
bool waitCurve() { return waitFor(motion::curveDone); }

bool straight(float distance, float topSpeed, float finalSpeed, float accel, bool steering) {
  motion::setSteering(steering);
  motion::extendForward(distance, topSpeed, finalSpeed, accel);
  return waitForward();
}

bool turnInPlace(Turn t, float omega, float alpha) {
  float angle = 0;
  switch (t) {
    case mm::LEFT: angle = 90; break;
    case mm::RIGHT: angle = -90; break;
    case mm::BACK: angle = 180; break;
    default: return true;
  }
  motion::setSteering(false);
  motion::startTurn(angle, omega, alpha);
  bool ok = waitTurn();
  delay(30);  // cho robot on dinh
  return ok;
}

static bool curve(const mm::TurnParams& p, int8_t dir) {
  motion::setSteering(false);
  motion::startCurve(p, dir);
  return waitCurve();
}

void alignToFrontWall() {
  // Dung cam bien truoc: tong -> khoang cach, hieu -> goc. Chi chay khi co tuong truoc.
  if (!sensors::frontWall()) return;
  motion::setSteering(false);
  const uint32_t t0 = millis();
  while (millis() - t0 < ALIGN_TIME_MS) {
    const float distErr = FRONT_REFERENCE - sensors::frontSum();  // >0: con xa tuong
    const float angErr = sensors::rfs - sensors::lfs;             // >0: lech trai
    motion::setManualSpeeds(constrain(ALIGN_FWD_KP * distErr, -80.0f, 80.0f),
                            constrain(-ALIGN_ROT_KP * angErr, -90.0f, 90.0f));
    if (aborted()) break;
    delay(2);
  }
  motion::stopManual();
}

// ----------------------------------------------------------------- RobotIO (tham do)
void RobotIO::senseWalls(bool& left, bool& front, bool& right) {
  left = sensors::leftWall();
  right = sensors::rightWall();
  front = sensors::frontWall();
  if (pose_ == AT_START && START_CELL_HAS_EAST_WALL) right = true;
  lastFrontWall_ = front;
}

bool RobotIO::move(Turn t) {
  const SearchParams& p = params_;
  const float v = p.speed, a = p.accel;
  bool ok = true;
  switch (pose_) {
    case AT_START:
      motion::resetProfiles();
      if (t == mm::AHEAD) {
        ok = straight(START_TO_EDGE + SENSING_OFFSET, v, v, a);
      } else {
        ok = straight(START_TO_CENTER, v, 0, a) && turnInPlace(t, p.turnSpeed, p.turnAccel) &&
             straight(HALF_CELL + SENSING_OFFSET, v, v, a);
      }
      break;
    case AT_CENTER:
      ok = turnInPlace(t, p.turnSpeed, p.turnAccel) && straight(HALF_CELL + SENSING_OFFSET, v, v, a);
      break;
    case AT_SENSING:
      if (t == mm::AHEAD) {
        ok = straight(CELL_PITCH, v, v, a);
      } else if (t != mm::BACK && gSmoothSearch) {
        // Re cong SS90 khong dung: thang -> cong -> thang toi diem doc cam bien o moi
        const mm::TurnParams& ss = gSearchTurn;
        ok = straight(ss.before - SENSING_OFFSET, v, v, a) && curve(ss, t == mm::LEFT ? 1 : -1) &&
             straight(ss.after + SENSING_OFFSET, v, v, a);
      } else {
        // toi tam o, dung, (can chinh tuong truoc), quay tai cho, chay tiep
        ok = straight(HALF_CELL - SENSING_OFFSET, v, 0, a);
        if (ok && lastFrontWall_) alignToFrontWall();
        ok = ok && turnInPlace(t, p.turnSpeed, p.turnAccel) && straight(HALF_CELL + SENSING_OFFSET, v, v, a);
      }
      break;
  }
  pose_ = AT_SENSING;
  return ok;
}

void RobotIO::stopAtCenter() {
  if (pose_ == AT_SENSING) {
    straight(HALF_CELL - SENSING_OFFSET, params_.speed, 0, params_.accel);
    if (lastFrontWall_) alignToFrontWall();
  }
  motion::setSteering(false);
  pose_ = AT_CENTER;
}

bool RobotIO::shouldAbort() {
  if (timeUp()) {  // het gio: dung lai tai cho (o tam o), giu ban do
    stopAtCenter();
    return true;
  }
  return aborted();
}

// ----------------------------------------------------------------- Speed run slalom
namespace {
struct ExecCtx {
  const mm::RunConfig* cfg;
};

bool execSegment(const mm::Segment& s, void* vctx) {
  const mm::RunConfig& cfg = *static_cast<ExecCtx*>(vctx)->cfg;
  switch (s.kind) {
    case mm::SEG_STRAIGHT:
      // Bam tuong ben chi khi chay truc giao (doan cheo khong co tuong song song)
      return straight(s.length, s.vmax, s.vend, cfg.accel, !s.diagonal);
    case mm::SEG_CURVE:
      return curve(cfg.turn[s.turn], s.dir);
    case mm::SEG_INPLACE: {
      const Turn t = s.dir == 2 ? mm::BACK : (s.dir > 0 ? mm::LEFT : mm::RIGHT);
      return turnInPlace(t, SEARCH_PARAMS.turnSpeed, SEARCH_PARAMS.turnAccel);
    }
  }
  return true;
}
}  // namespace

bool runSmooth(const mm::SmoothMove* moves, const mm::RunConfig& cfg) {
  motion::resetProfiles();
  ExecCtx ctx{&cfg};
  const bool ok = mm::walkSegments(moves, cfg, execSegment, &ctx);
  motion::setSteering(false);
  return ok;
}

// ----------------------------------------------------------------- Xuat phat
void waitForHandStart() {
  sensors::setEnabled(true);
  ui::led(true);
  while (sensors::frontSum() < START_HAND_THRESHOLD) delay(5);  // dua tay vao
  ui::beep(30);
  while (sensors::frontSum() > START_HAND_THRESHOLD * 0.5f) delay(5);  // rut tay ra
  ui::led(false);
  delay(500);
}

}  // namespace robot
