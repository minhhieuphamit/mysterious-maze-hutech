// =============================================================================
//  Test MazeCore tren may tinh (khong can robot, khong can mms).
//
//  - Sinh ngau nhien N me cung 16x16 (co vong lap, dich 2x2 giua) va cho chuot
//    tham do bang dung code Explorer cua firmware, kiem tra:
//      * luon toi duoc dich, quay ve duoc start, khong dam tuong
//      * duong speed run hop le va so voi duong toi uu (biet truoc toan bo me cung)
//  - Neu truyen file me cung (dinh dang micromouseonline/mazefiles) thi chay tren file do.
//
//  Build:  xem test/host/build.ps1 hoac README.
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "explorer.h"
#include "maze.h"
#include "planner.h"

using namespace mm;

static int gFailures = 0;
#define CHECK(cond, msg)                                              \
  do {                                                                \
    if (!(cond)) {                                                    \
      std::printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); \
      gFailures++;                                                    \
    }                                                                 \
  } while (0)

#include "maze_gen.h"

// ---------------------------------------------------------------- IO gia lap
class FakeIO : public MouseIO {
 public:
  explicit FakeIO(const Maze& truth) : truth_(truth) {}
  void senseWalls(bool& l, bool& f, bool& r) override {
    l = truth_.hasWall(loc, leftOf(heading));
    f = truth_.hasWall(loc, heading);
    r = truth_.hasWall(loc, rightOf(heading));
  }
  bool move(Turn t) override {
    heading = applyTurn(heading, t);
    if (t != AHEAD) turns++;
    if (truth_.hasWall(loc, heading)) {
      crashes++;
      return false;
    }
    loc = loc.neighbour(heading);
    cells++;
    return true;
  }
  void stopAtCenter() override { stops++; }

  Location loc{0, 0};
  Heading heading = NORTH;
  int cells = 0, turns = 0, stops = 0, crashes = 0;

 private:
  const Maze& truth_;
};

// Chay chuoi lenh speed run tren me cung that, tra ve true neu toi dich ma khong dam tuong.
static bool replayMoves(const Maze& truth, const char* moves) {
  Location p{0, 0};
  Heading d = NORTH;
  for (const char* c = moves; *c; c++) {
    switch (*c) {
      case 'l': d = leftOf(d); break;
      case 'r': d = rightOf(d); break;
      case 'a': d = behind(d); break;
      case 'L': d = leftOf(d); break;
      case 'R': d = rightOf(d); break;
      default: break;
    }
    if (*c == 'B' || *c == 'F' || *c == 'L' || *c == 'R') {
      if (truth.hasWall(p, d)) return false;
      p = p.neighbour(d);
    }
  }
  return truth.isGoal(p);
}

struct RunStats {
  bool ok = false;
  int toGoal = 0, total = 0;
  bool solved = false;
  bool optimal = false;
};

static RunStats runOne(const Maze& truth, bool verbose) {
  RunStats st;
  Maze mouseMap;
  // Goal cua chuot lay theo goal cua me cung that (file co the khac 2x2)
  mouseMap.clearGoals();
  for (uint8_t i = 0; i < truth.goalCount(); i++) mouseMap.addGoal(truth.goals()[i]);

  FakeIO io(truth);
  Explorer ex(mouseMap, io);
  ex.setPose({0, 0}, NORTH);
  PlannerCosts costs;

  if (ex.searchToGoals() != SearchResult::REACHED) {
    CHECK(false, "khong toi duoc dich");
    return st;
  }
  CHECK(truth.isGoal(io.loc), "vi tri IO khong khop goal");
  st.toGoal = io.cells;
  st.solved = ex.exploreUntilSolved(costs, 300);
  CHECK(io.loc == (Location{0, 0}), "khong quay ve start");
  CHECK(io.crashes == 0, "dam tuong khi tham do");
  st.total = io.cells;

  char moves[kCellCount * 2];
  uint32_t known = 0, best = 0;
  if (!buildSpeedRun(mouseMap, NORTH, costs, moves, sizeof(moves), &known)) {
    CHECK(false, "khong co duong speed run");
    return st;
  }
  CHECK(replayMoves(truth, moves), "duong speed run dam tuong / khong toi dich");
  char bestMoves[kCellCount * 2];
  buildSpeedRun(truth, NORTH, costs, bestMoves, sizeof(bestMoves), &best);
  st.optimal = known == best;
  if (st.solved) CHECK(st.optimal, "bao 'solved' nhung khong toi uu");
  if (verbose) {
    std::printf("  toi dich: %d o | tong tham do: %d o | solved=%d | cost %u (toi uu %u)\n", st.toGoal, st.total,
                st.solved, unsigned(known), unsigned(best));
    std::printf("  lenh speed run: %s\n", moves);
  }
  st.ok = true;
  return st;
}

// ---------------------------------------------------------------- Unit test nho
static void testBasics() {
  std::printf("[unit] flood fill & planner\n");
  Maze m;
  CHECK(m.goalCount() == (kMazeSize % 2 ? 1 : 4), "so o dich");
  m.floodToGoals(Maze::OPEN);
  // Me cung trong: cost(0,0) = khoang cach Manhattan toi o dich gan nhat
  uint16_t expect = uint16_t(2 * (kMazeSize / 2 - 1));
  CHECK(m.cost({0, 0}) == expect, "cost Manhattan me cung trong");
  m.setWall({0, 0}, EAST, true);
  CHECK(m.hasWall({1, 0}, WEST), "tuong cap nhat 2 phia");
  CHECK(m.isKnown({1, 0}, WEST), "tuong da biet 2 phia");
  CHECK(!m.isExit({0, 0}, WEST, Maze::OPEN), "tuong bao");
  CHECK(!m.isExit({0, 0}, NORTH, Maze::CLOSED), "CLOSED: tuong chua biet = chan");
  CHECK(m.isExit({0, 0}, NORTH, Maze::OPEN), "OPEN: tuong chua biet = mo");
  Heading h;
  m.floodToGoals(Maze::OPEN);
  CHECK(m.bestHeading({0, 0}, NORTH, Maze::OPEN, h) && h == NORTH, "bestHeading");

  // Planner: me cung mo hoan toan (OPEN) -> duong it re nhat: thang roi re 1 lan
  Location path[kCellCount];
  PlannerCosts costs;
  uint32_t c = 0;
  uint16_t n = planPath(m, {0, 0}, NORTH, m.goals(), m.goalCount(), Maze::OPEN, costs, path, kCellCount, &c);
  CHECK(n == expect + 1, "planner do dai");
  CHECK(c == expect * costs.straight + costs.turn, "planner chi re 1 lan");
  char mv[64];
  buildMoves(path, n, NORTH, mv, sizeof(mv));
  std::printf("  moves me cung trong: %s\n", mv);
}

// ---------------------------------------------------------------- Ban do goi y nap san
static void wallsOf(const Maze& m, uint8_t* out) {
  for (uint16_t i = 0; i < kCellCount; i++) {
    Location c = Location::fromIndex(i);
    uint8_t b = 0;
    for (uint8_t h = 0; h < 4; h++)
      if (m.hasWall(c, Heading(h))) b |= uint8_t(1u << h);
    out[i] = b;
  }
}

// Dao ngau nhien k tuong ben trong (mo phong ban do nap san bi sai)
static Maze flipWalls(const Maze& m, int k, uint32_t seed) {
  Maze r = m;
  std::mt19937 rng(seed);
  for (int n = 0; n < k;) {
    Location c{uint8_t(rng() % kMazeSize), uint8_t(rng() % kMazeSize)};
    Heading h = (rng() & 1) ? NORTH : EAST;
    if (!c.neighbour(h).isValid()) continue;
    r.setWall(c, h, !r.hasWall(c, h));
    n++;
  }
  return r;
}

struct PresetStats {
  bool ok = false;
  int toGoal = 0, total = 0, mismatch = 0;
  bool rejected = false, optimal = false;
};

// hint = nullptr: tham do binh thuong
static PresetStats runPreset(const Maze& truth, const Maze* hint) {
  PresetStats st;
  Maze map;
  map.clearGoals();
  for (uint8_t i = 0; i < truth.goalCount(); i++) map.addGoal(truth.goals()[i]);
  if (hint) {
    uint8_t w[kCellCount];
    wallsOf(*hint, w);
    map.setHint(w);
  }
  FakeIO io(truth);
  Explorer ex(map, io);
  ex.setPose({0, 0}, NORTH);
  if (ex.searchToGoals() != SearchResult::REACHED) {
    CHECK(false, "[preset] khong toi dich");
    return st;
  }
  st.toGoal = io.cells;
  PlannerCosts pc;
  ex.exploreUntilSolved(pc, 300);
  CHECK(io.crashes == 0, "[preset] dam tuong");
  CHECK(io.loc == (Location{0, 0}), "[preset] khong ve start");
  st.total = io.cells;
  st.mismatch = ex.hintMismatches();
  st.rejected = ex.hintRejected();
  char mv[kCellCount * 2], best[kCellCount * 2];
  uint32_t known = 0, opt = 0;
  if (!buildSpeedRun(map, NORTH, pc, mv, sizeof(mv), &known)) {
    CHECK(false, "[preset] khong co duong speed run");
    return st;
  }
  // An toan: duong speed run chi qua tuong DA DO -> phai chay duoc tren me cung that
  CHECK(replayMoves(truth, mv), "[preset] speed run dam tuong");
  buildSpeedRun(truth, NORTH, pc, best, sizeof(best), &opt);
  st.optimal = known == opt;
  st.ok = true;
  return st;
}

static void testPresets(const Maze& truth, const char* name) {
  std::printf("[ban do goi y] %s\n", name);
  Maze other;
  generateConnectedMaze(other, 999, 0.08);
  struct Case {
    const char* label;
    const Maze* hint;
    Maze store;
  };
  Case cases[4] = {{"khong nap map (tham do binh thuong)", nullptr, Maze()},
                   {"nap map DUNG", nullptr, truth},
                   {"nap map sai 6 tuong", nullptr, flipWalls(truth, 6, 7)},
                   {"nap map cua me cung KHAC", nullptr, other}};
  for (int i = 1; i < 4; i++) cases[i].hint = &cases[i].store;
  for (const Case& c : cases) {
    PresetStats st = runPreset(truth, c.hint);
    std::printf("  %-38s toi dich %3d o | tong %3d o | lech %2d tuong%s | speed run %s\n", c.label, st.toGoal,
                st.total, st.mismatch, st.rejected ? " -> BO MAP" : "", st.optimal ? "toi uu" : "CHUA toi uu");
  }
}

// Header nap vao firmware (src/preset_maze.h) phai trung voi file me cung nguon.
#include "../../src/preset_maze.h"
static void testPresetHeader() {
  if (!PRESET_MAZE_AVAILABLE) return;
  const std::string path = std::string("mazes/") + PRESET_MAZE_NAME;
  Maze file;
  if (!loadMazeFile(path, file)) {
    std::printf("[preset_maze.h] bo qua: khong mo duoc %s (chay test tu thu muc goc project)\n", path.c_str());
    return;
  }
  uint8_t w[kCellCount];
  wallsOf(file, w);
  int diff = 0;
  for (uint16_t i = 0; i < kCellCount; i++) diff += (w[i] != PRESET_MAZE_WALLS[i]);
  std::printf("[preset_maze.h] %s: %d o khac voi file\n", PRESET_MAZE_NAME, diff);
  CHECK(diff == 0, "src/preset_maze.h khong khop file me cung");
}

int main(int argc, char** argv) {
  testBasics();
  testPresetHeader();

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      Maze truth;
      if (!loadMazeFile(argv[i], truth)) {
        std::printf("[file] khong doc duoc %s\n", argv[i]);
        gFailures++;
        continue;
      }
      std::printf("[file] %s\n", argv[i]);
      runOne(truth, true);
      testPresets(truth, argv[i]);
    }
  } else {
    const int kRuns = 300;
    std::printf("[random] %d me cung ngau nhien %dx%d\n", kRuns, kMazeSize, kMazeSize);
    int ok = 0, solved = 0, optimal = 0;
    long sumGoal = 0, sumTotal = 0;
    for (int s = 1; s <= kRuns; s++) {
      Maze truth;
      generateConnectedMaze(truth, uint32_t(s), 0.08);
      RunStats st = runOne(truth, s == 1);
      if (!st.ok) {
        std::printf("  -> seed %d loi\n", s);
        continue;
      }
      ok++;
      solved += st.solved;
      optimal += st.optimal;
      sumGoal += st.toGoal;
      sumTotal += st.total;
    }
    std::printf("  thanh cong %d/%d | ban do toi uu %d/%d | duong speed run toi uu %d/%d\n", ok, kRuns, solved,
                kRuns, optimal, kRuns);
    if (ok) std::printf("  trung binh: %ld o toi dich, %ld o tong tham do\n", sumGoal / ok, sumTotal / ok);

    // Ban do goi y tren 100 me cung: dung / sai 6 tuong / me cung khac
    std::printf("[ban do goi y] 100 me cung ngau nhien (trung binh)\n");
    const char* labels[4] = {"khong nap map", "nap map DUNG", "nap map sai 6 tuong", "nap map me cung KHAC"};
    for (int k = 0; k < 4; k++) {
      long g = 0, t = 0, opt = 0, rej = 0, okc = 0;
      for (int sd = 1; sd <= 100; sd++) {
        Maze truth;
        generateConnectedMaze(truth, uint32_t(sd), 0.08);
        Maze hint = truth;
        if (k == 2) hint = flipWalls(truth, 6, uint32_t(sd));
        if (k == 3) generateConnectedMaze(hint, uint32_t(sd + 5000), 0.08);
        PresetStats st = runPreset(truth, k == 0 ? nullptr : &hint);
        if (!st.ok) continue;
        okc++;
        g += st.toGoal;
        t += st.total;
        opt += st.optimal;
        rej += st.rejected;
      }
      std::printf("  %-22s thanh cong %ld/100 | %3ld o toi dich | %3ld o tong | speed run toi uu %ld/100 | bo map %ld\n",
                  labels[k], okc, g / 100, t / 100, opt, rej);
    }
  }

  std::printf(gFailures ? "\nKET QUA: %d LOI\n" : "\nKET QUA: OK\n", gFailures);
  return gFailures ? 1 : 0;
}
