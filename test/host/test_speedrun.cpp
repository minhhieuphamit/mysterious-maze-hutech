// =============================================================================
//  Test luot chay toc do: dich duong (slalom/cheo), hinh hoc re, planner theo thoi
//  gian, va MO PHONG DONG HOC robot chay that theo duong cong -> do khe ho voi
//  tuong/cot cua me cung that.
// =============================================================================
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "explorer.h"
#include "maze.h"
#include "planner.h"
#include "run_defaults.h"
#include "smooth_path.h"

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

constexpr float kPi = 3.14159265358979f;
constexpr float kHalf = 96.0f;
constexpr float kWallHalf = 6.0f;
static float gRobotRadius = 45.0f;  // ban kinh vong tron bao than robot (mm) khi tinh khe ho

static std::string names(const SmoothMove* m) {
  std::string s;
  char buf[16];
  for (; m->kind != MK_END; m++) {
    if (!s.empty()) s += ' ';
    s += moveName(*m, buf, sizeof(buf));
  }
  return s;
}

// ------------------------------------------------- 1. Doi chieu voi test cua Bulebule
static void testTranslation() {
  std::printf("[smooth] doi chieu bo dich voi test_path.py cua Bulebule\n");
  struct Case {
    const char* sharp;
    const char* expect;
  };
  // Ten Bulebule -> ten o day: LEFT_90=F90L, LEFT_TO_45=D45inL, RIGHT_FROM_45=D45outR,
  // LEFT_TO_135=D135inL, LEFT_FROM_135=D135outL, LEFT_DIAGONAL=V90L, DIAGONAL=D, LEFT=SS90L
  const Case diag[] = {
      {"FLF", "F F90L F"},
      {"FLLF", "F F180L F"},
      {"FLRF", "F D45inL D45outR F"},
      {"FLRLF", "F D45inL D D45outL F"},
      {"FLLRRF", "F D135inL D135outR F"},
      {"FLLRLLF", "F D135inL D D135outL F"},
      {"FLRRF", "F D45inL D135outR F"},
      {"FLLRF", "F D135inL D45outR F"},
      {"FRLLRF", "F D45inR V90L D45outR F"},
      {"FFRLRLRRFRLLRRFRFFLRRLRRFRLRLFF",
       "F F D45inR D D D D135outR F D45inR V90L D135outR F F90R F F D45inL V90R D D135outR F D45inR D D D45outL F "
       "F"},
  };
  SmoothMove out[128];
  for (const Case& c : diag) {
    smoothPath(c.sharp, LANG_DIAGONALS, out, 128);
    std::string got = names(out);
    if (got != c.expect) std::printf("  %s -> %s (mong doi %s)\n", c.sharp, got.c_str(), c.expect);
    CHECK(got == c.expect, "dich DIAGONALS");
  }
  const Case safe[] = {{"FLRF", "F SS90L SS90R F"}, {"FRLLF", "F SS90R SS90L SS90L F"}, {"FLLF", "F F180L F"}};
  for (const Case& c : safe) {
    smoothPath(c.sharp, LANG_SAFE, out, 128);
    std::string got = names(out);
    if (got != c.expect) std::printf("  %s -> %s (mong doi %s)\n", c.sharp, got.c_str(), c.expect);
    CHECK(got == c.expect, "dich SAFE");
  }
  // 'S' duoc coi nhu 'F' khi nhin truoc
  smoothPath("BFLS", LANG_DIAGONALS, out, 128);
  CHECK(names(out) == "START F F90L STOP", "S nhu F");
}

// ------------------------------------------------- 2. Hinh hoc re
static void testTurnGeometry(const RunConfig& cfg) {
  std::printf("[turns] hinh hoc re (o 192 mm) - before/after tinh tu dong\n");
  std::printf("  %-8s %6s %7s %7s %7s %8s %8s %7s\n", "kieu", "goc", "R(mm)", "trans", "arc", "before", "after",
              "v(m/s)");
  for (uint8_t i = 0; i < TURN_TYPE_COUNT; i++) {
    const TurnParams& p = cfg.turn[i];
    std::printf("  %-8s %6.0f %7.1f %7.1f %7.1f %8.1f %8.1f %7.2f\n", turnName(TurnType(i)), p.angle, p.radius,
                p.transition, p.arc, p.before, p.after, cfg.turnSpeed[i] / 1000.0f);
    CHECK(p.valid, "turn params");
    // Kiem tra: tich phan lai quy dao (pre + curve + post) phai trung diem moc.
    float X, Y, ang;
    turnTarget(TurnType(i), kHalf, X, Y, ang);
    float x = p.before, y = 0, phi = 0;
    const int N = 4000;
    const float ds = p.length() / N;
    for (int k = 0; k < N; k++) {
      float kk = curveShape(p, (k + 0.5f) * ds) / p.radius;
      float mid = phi + 0.5f * kk * ds;
      x += std::cos(mid) * ds;
      y += std::sin(mid) * ds;
      phi += kk * ds;
    }
    x += p.after * std::cos(phi);
    y += p.after * std::sin(phi);
    const float err = std::hypot(x - X, y - Y);
    const float angErr = std::fabs(phi * 180 / kPi - ang);
    CHECK(err < 0.5f && angErr < 0.1f, "quy dao re khong trung diem moc");
  }
}

// ------------------------------------------------- 3. Mo phong dong hoc + khe ho
struct Rect {
  float x0, y0, x1, y1;
};
static float distToRect(float px, float py, const Rect& r) {
  float dx = std::fmax(std::fmax(r.x0 - px, 0.0f), px - r.x1);
  float dy = std::fmax(std::fmax(r.y0 - py, 0.0f), py - r.y1);
  return std::hypot(dx, dy);
}

struct SimCtx {
  const Maze* truth;
  const RunConfig* cfg;
  float x, y, phi;  // tam o (i,j) = (2h*i, 2h*j)
  float minClear = 1e9f;
  TurnType worstTurn = TURN_TYPE_COUNT;
  bool inCurve = false;
  TurnType curTurn = TURN_SS90;
  float traveled = 0;
  struct Pt {
    float x, y;
    uint8_t kind;  // 0 thang truc giao, 1 thang cheo, 2 duong cong
  };
  std::vector<Pt>* trail = nullptr;
  void record(uint8_t kind) {
    if (trail) trail->push_back(Pt{x, y, kind});
  }

  void check() {
    if (traveled < 30.0f) return;  // tu the xuat phat: lung robot cham tuong sau (co y)
    // Khe ho = khoang cach toi tuong / cot gan nhat - ban kinh robot
    const int ci = int(std::floor((x + kHalf) / (2 * kHalf)));
    const int cj = int(std::floor((y + kHalf) / (2 * kHalf)));
    float best = 1e9f;
    for (int i = ci - 1; i <= ci + 1; i++)
      for (int j = cj - 1; j <= cj + 1; j++) {
        // cot o goc tren-phai cua o (i,j)
        const float cx = (2 * i + 1) * kHalf, cy = (2 * j + 1) * kHalf;
        best = std::fmin(best, distToRect(x, y, Rect{cx - kWallHalf, cy - kWallHalf, cx + kWallHalf, cy + kWallHalf}));
        if (i < 0 || j < 0 || i >= kMazeSize || j >= kMazeSize) continue;
        const Location c{uint8_t(i), uint8_t(j)};
        const float x0 = (2 * i - 1) * kHalf, x1 = (2 * i + 1) * kHalf;
        const float y0 = (2 * j - 1) * kHalf, y1 = (2 * j + 1) * kHalf;
        if (truth->hasWall(c, NORTH)) best = std::fmin(best, distToRect(x, y, Rect{x0, y1 - kWallHalf, x1, y1 + kWallHalf}));
        if (truth->hasWall(c, SOUTH)) best = std::fmin(best, distToRect(x, y, Rect{x0, y0 - kWallHalf, x1, y0 + kWallHalf}));
        if (truth->hasWall(c, EAST)) best = std::fmin(best, distToRect(x, y, Rect{x1 - kWallHalf, y0, x1 + kWallHalf, y1}));
        if (truth->hasWall(c, WEST)) best = std::fmin(best, distToRect(x, y, Rect{x0 - kWallHalf, y0, x0 + kWallHalf, y1}));
      }
    const float clear = best - gRobotRadius;
    if (clear < minClear) {
      minClear = clear;
      worstTurn = inCurve ? curTurn : TURN_TYPE_COUNT;
    }
  }
};

static bool simSink(const Segment& s, void* vctx) {
  SimCtx& c = *static_cast<SimCtx*>(vctx);
  const float step = 2.0f;
  switch (s.kind) {
    case SEG_STRAIGHT: {
      c.inCurve = false;
      for (float d = 0; d < s.length; d += step) {
        const float ds = std::fmin(step, s.length - d);
        c.x += std::cos(c.phi) * ds;
        c.y += std::sin(c.phi) * ds;
        c.traveled += ds;
        c.check();
        c.record(s.diagonal ? 1 : 0);
      }
      break;
    }
    case SEG_CURVE: {
      c.inCurve = true;
      c.curTurn = s.turn;
      const TurnParams& p = c.cfg->turn[s.turn];
      const int N = int(p.length() / 0.5f) + 1;
      const float ds = p.length() / N;
      for (int k = 0; k < N; k++) {
        const float kk = s.dir * curveShape(p, (k + 0.5f) * ds) / p.radius;
        const float mid = c.phi + 0.5f * kk * ds;
        c.x += std::cos(mid) * ds;
        c.y += std::sin(mid) * ds;
        c.phi += kk * ds;
        c.traveled += ds;
        if (k % 4 == 0) {
          c.check();
          c.record(2);
        }
      }
      c.inCurve = false;
      break;
    }
    case SEG_INPLACE:
      c.phi += (s.dir == 2 ? kPi : s.dir * kPi / 2);
      break;
  }
  return true;
}

// O cuoi cung cua chuoi lenh
static Location finalCell(const char* moves) {
  Location p{0, 0};
  Heading h = NORTH;
  for (const char* c = moves; *c; c++) {
    switch (*c) {
      case 'l': h = leftOf(h); break;
      case 'r': h = rightOf(h); break;
      case 'a': h = behind(h); break;
      case 'L': h = leftOf(h); p = p.neighbour(h); break;
      case 'R': h = rightOf(h); p = p.neighbour(h); break;
      case 'B':
      case 'F': p = p.neighbour(h); break;
    }
  }
  return p;
}

class FakeIO : public MouseIO {
 public:
  explicit FakeIO(const Maze& t) : truth_(t) {}
  void senseWalls(bool& l, bool& f, bool& r) override {
    l = truth_.hasWall(loc, leftOf(heading));
    f = truth_.hasWall(loc, heading);
    r = truth_.hasWall(loc, rightOf(heading));
  }
  bool move(Turn t) override {
    heading = applyTurn(heading, t);
    if (truth_.hasWall(loc, heading)) return false;
    loc = loc.neighbour(heading);
    trail.push_back(loc);
    return true;
  }
  void stopAtCenter() override {}
  Location loc{0, 0};
  Heading heading = NORTH;
  std::vector<Location> trail{Location{0, 0}};

 private:
  const Maze& truth_;
};

// ------------------------------------------------- 4. Xuat hinh SVG (mo bang trinh duyet)
static void writeSvg(const char* file, const Maze& truth, const Maze& map, const std::vector<Location>& explore,
                     const std::vector<SimCtx::Pt>& run, const std::string& title, const std::string& sub) {
  const float sc = 0.25f;  // px / mm
  const float size = kMazeSize * 2 * kHalf;
  const float W = size * sc + 40, H = size * sc + 125;
  auto X = [&](float x) { return 20 + (x + kHalf) * sc; };
  auto Y = [&](float y) { return 105 + (size - (y + kHalf)) * sc; };
  FILE* f = std::fopen(file, "w");
  if (!f) return;
  std::fprintf(f,
               "<svg xmlns='http://www.w3.org/2000/svg' width='%.0f' height='%.0f' viewBox='0 0 %.0f %.0f' "
               "font-family='sans-serif'>\n",
               W, H, W, H);
  std::fprintf(f,
               "<style>.anim{stroke-dasharray:1;stroke-dashoffset:1;animation:draw var(--d) linear var(--w) "
               "forwards}@keyframes draw{to{stroke-dashoffset:0}}</style>\n");
  std::fprintf(f, "<rect width='100%%' height='100%%' fill='#1b1d22'/>\n");
  std::fprintf(f, "<text x='20' y='28' fill='#fff' font-size='18' font-weight='bold'>%s</text>\n", title.c_str());
  const size_t cut = sub.find(" | ");
  std::fprintf(f, "<text x='20' y='50' fill='#bbb' font-size='12'>%s</text>\n", sub.substr(0, cut).c_str());
  if (cut != std::string::npos)
    std::fprintf(f, "<text x='20' y='67' fill='#bbb' font-size='12'>%s</text>\n", sub.substr(cut + 3).c_str());
  std::fprintf(f,
               "<text x='20' y='88' font-size='12'><tspan fill='#4ea3ff'>&#9644; tham do</tspan>  "
               "<tspan fill='#ffd24a'>&#9644; speed run thang</tspan>  <tspan fill='#ff5c8a'>&#9644; doan cheo</tspan>  "
               "<tspan fill='#5cff9d'>&#9644; duong cong</tspan>  <tspan fill='#777'>o xam = da di qua</tspan></text>\n");
  // O
  for (uint8_t i = 0; i < kMazeSize; i++)
    for (uint8_t j = 0; j < kMazeSize; j++) {
      const Location c{i, j};
      const char* fill = truth.isGoal(c) ? "#3a2f12" : (map.isVisited(c) ? "#2c3038" : "#22252b");
      std::fprintf(f, "<rect x='%.1f' y='%.1f' width='%.1f' height='%.1f' fill='%s'/>\n", X((2 * i - 1) * kHalf),
                   Y((2 * j + 1) * kHalf), 2 * kHalf * sc, 2 * kHalf * sc, fill);
    }
  std::fprintf(f, "<text x='%.1f' y='%.1f' fill='#fff' font-size='14' text-anchor='middle'>S</text>\n", X(0), Y(-20));
  // Tuong that (trang) - ve canh Bac va Dong cua moi o + bien
  auto wall = [&](float x0, float y0, float x1, float y1) {
    std::fprintf(f, "<line x1='%.1f' y1='%.1f' x2='%.1f' y2='%.1f' stroke='#e8e8e8' stroke-width='3' "
                    "stroke-linecap='square'/>\n", X(x0), Y(y0), X(x1), Y(y1));
  };
  for (uint8_t i = 0; i < kMazeSize; i++)
    for (uint8_t j = 0; j < kMazeSize; j++) {
      const Location c{i, j};
      const float x0 = (2 * i - 1) * kHalf, x1 = (2 * i + 1) * kHalf, y0 = (2 * j - 1) * kHalf, y1 = (2 * j + 1) * kHalf;
      if (truth.hasWall(c, NORTH)) wall(x0, y1, x1, y1);
      if (truth.hasWall(c, EAST)) wall(x1, y0, x1, y1);
      if (j == 0) wall(x0, y0, x1, y0);
      if (i == 0) wall(x0, y0, x0, y1);
    }
  // Duong tham do (tam o -> tam o)
  std::fprintf(f, "<polyline class='anim' style='--d:6s;--w:0s' pathLength='1' fill='none' stroke='#4ea3ff' "
                  "stroke-width='2' stroke-opacity='0.55' points='");
  for (const Location& l : explore) std::fprintf(f, "%.1f,%.1f ", X(2 * kHalf * l.x), Y(2 * kHalf * l.y));
  std::fprintf(f, "'/>\n");
  // Quy dao speed run: tach thanh doan theo loai de to mau, ve tung doan (an hien dan)
  const char* colors[3] = {"#ffd24a", "#ff5c8a", "#5cff9d"};
  size_t i = 0;
  const float total = float(run.size());
  while (i < run.size()) {
    size_t j = i;
    while (j + 1 < run.size() && run[j + 1].kind == run[i].kind) j++;
    const float t0 = 6.5f + 4.0f * i / total, dur = std::fmax(0.05f, 4.0f * (j + 1 - i) / total);
    std::fprintf(f, "<polyline class='anim' style='--d:%.2fs;--w:%.2fs' pathLength='1' fill='none' stroke='%s' "
                    "stroke-width='3.5' stroke-linejoin='round' points='", dur, t0, colors[run[i].kind]);
    for (size_t k = (i > 0 ? i - 1 : i); k <= j; k++) std::fprintf(f, "%.1f,%.1f ", X(run[k].x), Y(run[k].y));
    std::fprintf(f, "'/>\n");
    i = j + 1;
  }
  std::fprintf(f, "</svg>\n");
  std::fclose(f);
}

// test_speedrun --svg out.svg [seed | file_me_cung.txt] [ban_kinh_than_mm]
// test_speedrun --svg out.svg [seed|file] [ban_kinh] [--hint file_ban_do_goi_y]  (che do SEARCH+MAP)
static int svgMode(int argc, char** argv, const RunConfig& cfg) {
  std::vector<std::string> pos;
  std::string hintFile;
  for (int i = 2; i < argc; i++) {
    if (std::strcmp(argv[i], "--hint") == 0 && i + 1 < argc)
      hintFile = argv[++i];
    else
      pos.push_back(argv[i]);
  }
  const std::string out = pos.size() > 0 ? pos[0] : "maze.svg";
  const std::string src = pos.size() > 1 ? pos[1] : "1";
  if (pos.size() > 2) gRobotRadius = float(std::atof(pos[2].c_str()));
  Maze truth;
  std::string name;
  if (!src.empty() && std::isdigit(static_cast<unsigned char>(src[0])) && src.find('.') == std::string::npos) {
    generateConnectedMaze(truth, uint32_t(std::atoi(src.c_str())), 0.08);
    name = "Me cung ngau nhien seed " + src;
  } else {
    if (!loadMazeFile(src, truth)) {
      std::printf("Khong doc duoc file me cung: %s\n", src.c_str());
      return 1;
    }
    name = src;
  }
  Maze map;
  map.clearGoals();
  for (uint8_t g = 0; g < truth.goalCount(); g++) map.addGoal(truth.goals()[g]);
  if (!hintFile.empty()) {
    Maze hint;
    if (!loadMazeFile(hintFile, hint)) {
      std::printf("Khong doc duoc file ban do goi y: %s\n", hintFile.c_str());
      return 1;
    }
    uint8_t w[kCellCount];
    for (uint16_t i = 0; i < kCellCount; i++) {
      w[i] = 0;
      for (uint8_t h = 0; h < 4; h++)
        if (hint.hasWall(Location::fromIndex(i), Heading(h))) w[i] |= uint8_t(1u << h);
    }
    map.setHint(w);
  }
  FakeIO io(truth);
  Explorer ex(map, io);
  ex.setPose({0, 0}, NORTH);
  if (ex.searchToGoals() != SearchResult::REACHED) {
    std::printf("Khong tim thay duong toi dich!\n");
    return 1;
  }
  const size_t toGoal = io.trail.size() - 1;
  PlannerCosts pc;
  const bool solved = ex.exploreUntilSolved(pc, 300);
  map.clearHint();  // speed run chi dung tuong da do (giong firmware)
  std::string mode = "SEARCH";
  if (!hintFile.empty()) {
    char b[200];
    std::snprintf(b, sizeof(b), "SEARCH+MAP, lech %u tuong%s", unsigned(ex.hintMismatches()),
                  ex.hintRejected() ? " -> DA BO MAP" : "");
    mode = b;
  }
  name = "[" + mode + "] " + name;

  static char moves[kCellCount + 8];
  static SmoothMove smooth[kMaxSmoothMoves];
  float tl[LANG_COUNT] = {0};
  for (int l = 0; l < LANG_COUNT; l++)
    buildFastRun(map, cfg, PathLanguage(l), moves, sizeof(moves), smooth, kMaxSmoothMoves, &tl[l]);
  float t = 0;
  if (!buildFastRun(map, cfg, LANG_DIAGONALS, moves, sizeof(moves), smooth, kMaxSmoothMoves, &t)) {
    std::printf("Khong co duong speed run!\n");
    return 1;
  }
  std::vector<SimCtx::Pt> pts;
  SimCtx c{&truth, &cfg, 0, -cfg.startToCenter, kPi / 2};
  c.trail = &pts;
  pts.push_back(SimCtx::Pt{c.x, c.y, 0});
  walkSegments(smooth, cfg, simSink, &c);

  char sub[300];
  std::snprintf(sub, sizeof(sub),
                "Tham do: %zu o toi dich, %zu o tong (%s) | Speed run uoc luong: %.2f s (chi SS90: %.2f s, F90/F180: "
                "%.2f s) | khe ho min %.1f mm (than R=%.0f)",
                toGoal, io.trail.size() - 1, solved ? "ban do toi uu" : "chua chac toi uu", t, tl[0], tl[1],
                c.minClear, gRobotRadius);
  writeSvg(out.c_str(), truth, map, io.trail, pts, name, sub);
  std::printf("%s\n%s\n", name.c_str(), sub);
  std::printf("Duong: %s\nSlalom: %s\n", moves, names(smooth).c_str());
  std::printf("Da ghi hinh: %s  (mo bang Chrome/Edge)\n", out.c_str());
  return 0;
}

int main(int argc, char** argv) {
  const bool svg = argc > 1 && std::strcmp(argv[1], "--svg") == 0;
  if (argc > 1 && !svg) gRobotRadius = float(std::atof(argv[1]));
  if (!svg) testTranslation();

  RunConfig cfg;
  cfg.halfCell = kHalf;
  cfg.startToCenter = 50;
  cfg.topSpeed = 2000;
  cfg.diagSpeed = 1500;
  cfg.accel = 5000;
  cfg.lateralAccel = 9000;
  cfg.maxTurnSpeed = 2000;
  CHECK(cfg.init(kDefaultTurnSpecs), "RunConfig init");
  if (svg) return svgMode(argc, argv, cfg);
  testTurnGeometry(cfg);

  // Cau hinh "cu": chi re tai cho (de so sanh)
  std::printf("[speedrun] 300 me cung: tham do bang Explorer -> planner -> mo phong dong hoc\n");
  std::printf("  ban kinh than robot khi tinh khe ho: %.0f mm\n", gRobotRadius);
  const int kRuns = 300;
  double sumT[LANG_COUNT] = {0}, sumBest = 0;
  int ok = 0, langUse[LANG_COUNT] = {0};
  float worstClear = 1e9f;
  TurnType worstTurn = TURN_TYPE_COUNT;
  for (int s = 1; s <= kRuns; s++) {
    Maze truth;
    generateConnectedMaze(truth, uint32_t(s), 0.08);
    Maze map;
    FakeIO io(truth);
    Explorer ex(map, io);
    ex.setPose({0, 0}, NORTH);
    if (ex.searchToGoals() != SearchResult::REACHED) {
      CHECK(false, "tham do loi");
      continue;
    }
    PlannerCosts pc;
    ex.exploreUntilSolved(pc, 300);

    static char moves[kCellCount + 8];
    static SmoothMove smooth[kMaxSmoothMoves];
    // Thoi gian uoc luong cho tung muc ngon ngu
    for (int l = 0; l < LANG_COUNT; l++) {
      float t = 0;
      uint16_t n = buildFastRun(map, cfg, PathLanguage(l), moves, sizeof(moves), smooth, kMaxSmoothMoves, &t);
      CHECK(n > 0 && t > 0, "buildFastRun");
      sumT[l] += t;
    }
    float t = 0;
    PathLanguage used;
    buildFastRun(map, cfg, LANG_DIAGONALS, moves, sizeof(moves), smooth, kMaxSmoothMoves, &t, &used);
    langUse[used]++;
    sumBest += t;

    // Mo phong dong hoc tren me cung THAT
    SimCtx c{&truth, &cfg, 0, -cfg.startToCenter, kPi / 2};
    CHECK(walkSegments(smooth, cfg, simSink, &c), "walkSegments");
    const Location fc = finalCell(moves);
    const float ex_ = 2 * kHalf * fc.x, ey = 2 * kHalf * fc.y;
    const float endErr = std::hypot(c.x - ex_, c.y - ey);
    CHECK(truth.isGoal(fc), "duong khong ket thuc o dich");
    if (endErr > 1.0f) std::printf("  seed %d: lech diem dung %.1f mm (%s)\n", s, endErr, moves);
    CHECK(endErr < 1.0f, "diem dung lech");
    if (c.minClear < worstClear) {
      worstClear = c.minClear;
      worstTurn = c.worstTurn;
    }
    if (c.minClear < 0) {
      std::printf("  seed %d: CHAM TUONG (khe ho %.1f mm, %s): %s\n", s, c.minClear,
                  c.worstTurn < TURN_TYPE_COUNT ? turnName(c.worstTurn) : "doan thang", names(smooth).c_str());
      gFailures++;
    }
    if (s == 1) {
      std::printf("  vd seed 1: %s\n  -> %s\n  uoc luong %.2f s\n", moves, names(smooth).c_str(), t);
    }
    ok++;
  }
  std::printf("  thoi gian TB (uoc luong, s): chi SS90=%.2f | SAFE (F90/F180)=%.2f | CHEO=%.2f | chon tot nhat=%.2f\n",
              sumT[0] / ok, sumT[1] / ok, sumT[2] / ok, sumBest / ok);
  std::printf("  ngon ngu duoc chon: SEARCH %d, SAFE %d, DIAGONALS %d\n", langUse[0], langUse[1], langUse[2]);
  std::printf("  khe ho nho nhat voi tuong/cot: %.1f mm (%s)\n", worstClear,
              worstTurn < TURN_TYPE_COUNT ? turnName(worstTurn) : "doan thang");

  std::printf(gFailures ? "\nKET QUA: %d LOI\n" : "\nKET QUA: OK\n", gFailures);
  return gFailures ? 1 : 0;
}
