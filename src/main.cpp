// =============================================================================
//  MYSTERIOUS MAZE 2026 - Firmware Micromouse (STM32 + IR + DC encoder)
//
//  Dieu khien: nut MODE chon che do (LED nhay N lan = che do N), nut GO de chay.
//  Che do chay: sau khi nhan GO, dua tay truoc cam bien truoc roi rut ra -> xuat phat.
//  Dang chay nhan GO = dung khan cap.
// =============================================================================
#include <Arduino.h>

#include "config.h"
#include "explorer.h"
#include "hardware.h"
#include "maze.h"
#include "motion.h"
#include "planner.h"
#include "robot.h"
#include "smooth_path.h"
#include "preset_maze.h"
#include "storage.h"

using namespace mm;

enum Mode : uint8_t {
  MODE_SEARCH = 0,     // Tham do -> dich -> tham do them -> ve start, luu ban do
  MODE_SEARCH_MAP,     // Nhu SEARCH nhung dung ban do nap san (src/preset_maze.h) lam GOI Y
  MODE_RUN1,           // Speed run an toan: chi re SS90
  MODE_RUN2,           // Speed run: them re lon F90 / F180
  MODE_RUN3,           // Speed run nhanh nhat: chay cheo 45/135/V90
  MODE_SENSORS,        // In gia tri cam bien / encoder / pin qua UART
  MODE_TEST_STRAIGHT,  // Chay thang 4 o -> calib duong kinh banh
  MODE_TEST_SPIN,      // Quay 360 do x 2 -> calib khoang cach banh
  MODE_PRINT_MAZE,     // In ban do da luu
  MODE_CLEAR_MAZE,     // Xoa ban do da luu
  MODE_COUNT
};

static const char* const kModeNames[MODE_COUNT] = {"SEARCH",    "SEARCH+MAP", "RUN1 SS90", "RUN2 F90",
                                                   "RUN3 DIAG", "SENSORS",    "STRAIGHT4", "SPIN 720",
                                                   "PRINT MAP", "CLEAR MAP"};

static Maze gMaze;
static uint8_t gMode = MODE_SEARCH;

static void printLine(const char* s) { DebugSerial.println(s); }

static PlannerCosts searchCosts() {
  PlannerCosts c;
  c.straight = PLAN_COST_STRAIGHT;
  c.turn = PLAN_COST_TURN;
  return c;
}

static bool batteryOk() {
  if (sensors::batteryVolts < BATTERY_LOW_VOLTS) {
    DebugSerial.print("PIN YEU: ");
    DebugSerial.println(sensors::batteryVolts);
    ui::beeps(5);
    return false;
  }
  return true;
}

static void startRun() {
  robot::waitForHandStart();
  encoders::reset();
  motion::resetProfiles();
  motion::enable();
}

static void endRun() {
  motion::disable();
  sensors::setEnabled(false);
}

// ----------------------------------------------------------------- SEARCH
static bool runSearch(bool exploreAfterGoal, bool usePreset = false) {
  if (!storage::loadMaze(gMaze)) gMaze.reset();  // tiep tuc tu ban do cu neu co
  if (usePreset) {
    if (PRESET_MAZE_AVAILABLE) {
      // Chi la GOI Y de chon huong: robot van do tuong that o moi o, sai nhieu thi tu bo.
      gMaze.setHint(PRESET_MAZE_WALLS);
      DebugSerial.print("Dung ban do goi y: ");
      DebugSerial.println(PRESET_MAZE_NAME);
    } else {
      DebugSerial.println("Chua nap ban do (preset_maze.h trong) -> tham do binh thuong");
    }
  }
  robot::RobotIO io(SEARCH_PARAMS);
  Explorer ex(gMaze, io);
  ex.setHintTolerance(PRESET_MISMATCH_TOLERANCE);
  ex.setPose(Location{0, 0}, NORTH);
  startRun();
  io.setDeadline(SEARCH_TIME_LIMIT_MS);

  const uint32_t t0 = millis();
  SearchResult r = ex.searchToGoals();
  const uint32_t tGoal = millis() - t0;
  if (r != SearchResult::REACHED) {
    endRun();
    storage::saveMaze(gMaze);  // van giu nhung gi da biet
    DebugSerial.println(r == SearchResult::NO_ROUTE ? "KHONG CO DUONG!" : "HUY / VA CHAM / HET GIO");
    ui::beeps(4);
    return false;
  }
  // Dang dung yen o tam o dich (dong ho da dung): luu ban do ngay, phong khi bi nhac len.
  motion::disable();
  storage::saveMaze(gMaze);
  ui::beeps(2);
  DebugSerial.print("Toi dich sau (ms): ");
  DebugSerial.println(tGoal);

  bool solved = false;
  if (exploreAfterGoal) {
    motion::resetProfiles();
    motion::enable();
    solved = ex.exploreUntilSolved(searchCosts(), EXPLORE_MAX_CELLS);
  }
  endRun();
  storage::saveMaze(gMaze);
  if (usePreset && PRESET_MAZE_AVAILABLE) {
    DebugSerial.print("Ban do goi y: lech ");
    DebugSerial.print(ex.hintMismatches());
    DebugSerial.println(ex.hintRejected() ? " tuong -> DA BO (me cung khac?)" : " tuong");
  }
  gMaze.clearHint();  // speed run chi dung tuong da do
  DebugSerial.println(solved ? "Ban do TOI UU, da luu." : "Da luu ban do (chua chac toi uu).");
  ui::beeps(solved ? 3 : 1);
  return true;
}

// ----------------------------------------------------------------- SPEED RUN
static void runSpeed(uint8_t index) {
  if (!storage::loadMaze(gMaze)) {
    DebugSerial.println("Chua co ban do -> chuyen sang SEARCH");
    runSearch(true);
    return;
  }
  const RunConfig& cfg = robot::runConfig(index);
  static char moves[kCellCount + 8];
  static SmoothMove smooth[kMaxSmoothMoves];
  float est = 0;
  PathLanguage lang = LANG_SEARCH;
  const uint32_t tPlan = millis();
  const uint16_t n = buildFastRun(gMaze, cfg, PathLanguage(RUN_PARAMS[index].maxLang), moves, sizeof(moves), smooth,
                                  kMaxSmoothMoves, &est, &lang);
  if (!n) {
    DebugSerial.println("Ban do chua co duong da biet toi dich -> SEARCH");
    runSearch(true);
    return;
  }
  DebugSerial.print("Plan (ms): ");
  DebugSerial.println(millis() - tPlan);
  DebugSerial.print("Duong: ");
  DebugSerial.println(moves);
  DebugSerial.print("Slalom: ");
  char buf[12];
  for (uint16_t i = 0; i < n; i++) {
    DebugSerial.print(moveName(smooth[i], buf, sizeof(buf)));
    DebugSerial.print(' ');
  }
  DebugSerial.println();
  DebugSerial.print("Uoc luong (ms): ");
  DebugSerial.print(uint32_t(est * 1000));
  DebugSerial.print("  muc ngon ngu: ");
  DebugSerial.println(int(lang));

  startRun();
  const uint32_t t0 = millis();
  const bool ok = robot::runSmooth(smooth, cfg);
  const uint32_t dt = millis() - t0;
  endRun();
  DebugSerial.print(ok ? "Speed run xong (ms): " : "Speed run LOI sau (ms): ");
  DebugSerial.println(dt);
  ui::beeps(ok ? 3 : 5);
}

// ----------------------------------------------------------------- TEST
static void runSensors() {
  sensors::setEnabled(true);
  encoders::reset();
  while (!ui::goPressed() && !ui::modePressed()) {
    char buf[160];
    // (printf cua newlib-nano khong in so thuc -> in so nguyen)
    snprintf(buf, sizeof(buf), "raw %4d %4d %4d %4d | val %4d %4d %4d %4d | wall L%d F%d R%d | enc %ld %ld | %dmV",
             int(sensors::rawLss), int(sensors::rawLfs), int(sensors::rawRfs), int(sensors::rawRss),
             int(sensors::lss), int(sensors::lfs), int(sensors::rfs), int(sensors::rss), sensors::leftWall(),
             sensors::frontWall(), sensors::rightWall(), long(encoders::leftCount), long(encoders::rightCount),
             int(sensors::batteryVolts * 1000));
    DebugSerial.println(buf);
    delay(100);
  }
  sensors::setEnabled(false);
  ui::waitRelease(PIN_BUTTON_GO);
  ui::waitRelease(PIN_BUTTON_MODE);
}

static void runTestStraight() {
  startRun();
  robot::straight(START_TO_CENTER + 4 * CELL_PITCH, SEARCH_PARAMS.speed, 0, SEARCH_PARAMS.accel);
  delay(200);
  DebugSerial.print("Encoder do duoc (mm): ");
  DebugSerial.println(encoders::distance);
  endRun();
}

static void runTestSpin() {
  startRun();
  motion::startTurn(720, SEARCH_PARAMS.turnSpeed, SEARCH_PARAMS.turnAccel);
  robot::waitTurn();
  delay(200);
  DebugSerial.print("Goc encoder (deg): ");
  DebugSerial.println(encoders::angle);
  endRun();
}

// ----------------------------------------------------------------- setup / loop
void setup() {
  DebugSerial.begin(DEBUG_BAUD);
  ui::begin();
  sensors::begin();
  encoders::begin();
  motors::begin();
  motion::begin();
  delay(100);
  DebugSerial.println("\n=== MYSTERIOUS MAZE 2026 ===");
  DebugSerial.print("Pin: ");
  DebugSerial.println(sensors::batteryVolts);
  ui::beeps(1);
  ui::blink(gMode + 1);
}

void loop() {
  if (ui::modePressed()) {
    ui::waitRelease(PIN_BUTTON_MODE);
    gMode = (gMode + 1) % MODE_COUNT;
    DebugSerial.print("Mode ");
    DebugSerial.print(gMode);
    DebugSerial.print(": ");
    DebugSerial.println(kModeNames[gMode]);
    ui::blink(gMode + 1);
  }
  if (ui::goPressed()) {
    ui::waitRelease(PIN_BUTTON_GO);
    ui::beep(100);
    switch (gMode) {
      case MODE_SEARCH:
        if (batteryOk()) runSearch(true);
        break;
      case MODE_SEARCH_MAP:
        if (batteryOk()) runSearch(true, true);
        break;
      case MODE_RUN1:
      case MODE_RUN2:
      case MODE_RUN3:
        if (batteryOk()) runSpeed(uint8_t(gMode - MODE_RUN1));
        break;
      case MODE_SENSORS: runSensors(); break;
      case MODE_TEST_STRAIGHT: runTestStraight(); break;
      case MODE_TEST_SPIN: runTestSpin(); break;
      case MODE_PRINT_MAZE:
        if (storage::loadMaze(gMaze))
          gMaze.print(printLine);
        else
          DebugSerial.println("Chua co ban do luu.");
        break;
      case MODE_CLEAR_MAZE:
        storage::clearMaze();
        gMaze.reset();
        DebugSerial.println("Da xoa ban do.");
        ui::beeps(2);
        break;
    }
    // Neu dung khan bang nut GO, cho nha nut de khong kich hoat lai
    ui::waitRelease(PIN_BUTTON_GO);
  }
  delay(10);
}
