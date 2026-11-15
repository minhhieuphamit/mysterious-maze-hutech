#include "planner.h"

#include <string.h>

namespace mm {

namespace {

constexpr uint16_t kNone = 0xFFFF;
constexpr uint16_t kBasicStates = kCellCount * kHeadingCount;
#ifdef MM_LOW_RAM
constexpr uint16_t kFastStates = 0;  // STM32F103 (20 KB RAM): tat planner theo thoi gian
#else
constexpr uint16_t kFastStates = kCellCount * kHeadingCount * 3;  // + nuoc di truoc (F/L/R)
#endif
constexpr uint16_t kMaxStates = kFastStates > kBasicStates ? kFastStates : kBasicStates;

// Bo nho tinh dung chung (khong dung heap cap phat dong).
uint16_t gDist[kMaxStates];
uint16_t gPrev[kMaxStates];
Location gPathScratch[kCellCount];

// ---- Min-heap co chi muc (Dijkstra O(E log V)) ----
class IndexedHeap {
 public:
  void reset(uint16_t n) {
    size_ = 0;
    for (uint16_t i = 0; i < n; i++) pos_[i] = kNone;
  }
  bool empty() const { return size_ == 0; }
  // Them moi hoac giam khoa (khoa = gDist[state]).
  void push(uint16_t s) {
    if (pos_[s] == kNone) {
      pos_[s] = size_;
      heap_[size_++] = s;
    }
    up(pos_[s]);
  }
  uint16_t pop() {
    uint16_t top = heap_[0];
    pos_[top] = kDone;
    size_--;
    if (size_) {
      heap_[0] = heap_[size_];
      pos_[heap_[0]] = 0;
      down(0);
    }
    return top;
  }
  bool done(uint16_t s) const { return pos_[s] == kDone; }

 private:
  static constexpr uint16_t kDone = 0xFFFE;
  void swap(uint16_t a, uint16_t b) {
    uint16_t t = heap_[a];
    heap_[a] = heap_[b];
    heap_[b] = t;
    pos_[heap_[a]] = a;
    pos_[heap_[b]] = b;
  }
  void up(uint16_t i) {
    while (i > 0) {
      uint16_t p = (i - 1) / 2;
      if (gDist[heap_[p]] <= gDist[heap_[i]]) break;
      swap(i, p);
      i = p;
    }
  }
  void down(uint16_t i) {
    for (;;) {
      uint16_t l = 2 * i + 1, r = l + 1, m = i;
      if (l < size_ && gDist[heap_[l]] < gDist[heap_[m]]) m = l;
      if (r < size_ && gDist[heap_[r]] < gDist[heap_[m]]) m = r;
      if (m == i) break;
      swap(i, m);
      i = m;
    }
  }
  uint16_t heap_[kMaxStates];
  uint16_t pos_[kMaxStates];
  uint16_t size_ = 0;
};
IndexedHeap gHeap;

bool inTargets(Location c, const Location* targets, uint8_t n) {
  for (uint8_t i = 0; i < n; i++)
    if (targets[i] == c) return true;
  return false;
}

inline void relax(uint16_t from, uint16_t to, uint32_t nd) {
  if (gHeap.done(to)) return;
  if (nd < gDist[to]) {
    gDist[to] = uint16_t(nd > 0xFFFE ? 0xFFFE : nd);
    gPrev[to] = from;
    gHeap.push(to);
  }
}

}  // namespace

// =============================================================================
//  Planner co ban: trang thai (o, huong)
// =============================================================================
uint16_t planPath(const Maze& maze, Location start, Heading startHeading, const Location* targets,
                  uint8_t targetCount, Maze::Mask mask, const PlannerCosts& costs, Location* path,
                  uint16_t maxLen, uint32_t* totalCost) {
  for (uint16_t i = 0; i < kBasicStates; i++) {
    gDist[i] = kMaxCost;
    gPrev[i] = kNone;
  }
  gHeap.reset(kBasicStates);
  const uint16_t s0 = uint16_t(start.index() * kHeadingCount + startHeading);
  gDist[s0] = 0;
  gHeap.push(s0);
  uint16_t goalState = kNone;

  while (!gHeap.empty()) {
    const uint16_t u = gHeap.pop();
    const Location c = Location::fromIndex(u / kHeadingCount);
    const Heading h = Heading(u % kHeadingCount);
    if (inTargets(c, targets, targetCount)) {
      goalState = u;
      break;
    }
    const uint32_t d = gDist[u];
    if (maze.isExit(c, h, mask))
      relax(u, uint16_t(c.neighbour(h).index() * kHeadingCount + h), d + costs.straight);
    const uint16_t base = uint16_t(c.index() * kHeadingCount);
    relax(u, uint16_t(base + leftOf(h)), d + costs.turn);
    relax(u, uint16_t(base + rightOf(h)), d + costs.turn);
  }

  if (goalState == kNone) return 0;
  if (totalCost) *totalCost = gDist[goalState];

  uint16_t n = 0;
  uint16_t lastCell = kNone;
  for (uint16_t s = goalState; s != kNone; s = gPrev[s]) {
    const uint16_t cell = s / kHeadingCount;
    if (cell != lastCell) {
      if (n >= maxLen) return 0;
      path[n++] = Location::fromIndex(cell);
      lastCell = cell;
    }
  }
  for (uint16_t i = 0; i < n / 2; i++) {
    Location t = path[i];
    path[i] = path[n - 1 - i];
    path[n - 1 - i] = t;
  }
  return n;
}

uint16_t buildMoves(const Location* path, uint16_t count, Heading startHeading, char* out, uint16_t maxLen) {
  uint16_t n = 0;
  auto put = [&](char c) {
    if (n + 1 < maxLen) out[n++] = c;
  };
  if (count >= 2) {
    Heading h = startHeading;
    Heading first = headingBetween(path[0], path[1]);
    switch (turnBetween(h, first)) {
      case LEFT: put('l'); break;
      case RIGHT: put('r'); break;
      case BACK: put('a'); break;
      default: break;
    }
    h = first;
    put('B');
    for (uint16_t i = 1; i < count; i++) {
      if (i == count - 1) {
        put('S');
        break;
      }
      Heading next = headingBetween(path[i], path[i + 1]);
      switch (turnBetween(h, next)) {
        case AHEAD: put('F'); break;
        case LEFT: put('L'); break;
        case RIGHT: put('R'); break;
        case BACK: put('A'); break;  // khong xay ra voi duong ngan nhat
      }
      h = next;
    }
  }
  if (maxLen) out[n] = 0;
  return n;
}

// Neu o dich cuoi cung con 1 o dich nua thang phia truoc (vung dich 2x2 thong), chay
// them 1 o roi moi dung: robot vao vung dich (dung dong ho) voi toc do cao hon.
static void extendIntoGoal(const Maze& maze, char* moves, uint16_t& n, uint16_t maxLen) {
  if (n < 2 || moves[n - 1] != 'S' || n + 2 > maxLen) return;
  // Mo phong de lay o cuoi + huong
  Location p{0, 0};
  Heading h = NORTH;
  for (uint16_t i = 0; i + 1 < n; i++) {
    switch (moves[i]) {
      case 'l': h = leftOf(h); break;
      case 'r': h = rightOf(h); break;
      case 'a': h = behind(h); break;
      case 'L': h = leftOf(h); p = p.neighbour(h); break;
      case 'R': h = rightOf(h); p = p.neighbour(h); break;
      case 'B':
      case 'F': p = p.neighbour(h); break;
    }
  }
  if (!maze.isGoal(p) || !maze.isExit(p, h, Maze::CLOSED) || !maze.isGoal(p.neighbour(h))) return;
  moves[n - 1] = 'F';
  moves[n++] = 'S';
  moves[n] = 0;
}

uint16_t buildSpeedRun(const Maze& maze, Heading startHeading, const PlannerCosts& costs, char* out,
                       uint16_t maxLen, uint32_t* totalCost) {
  uint16_t count = planPath(maze, Location{0, 0}, startHeading, maze.goals(), maze.goalCount(), Maze::CLOSED,
                            costs, gPathScratch, kCellCount, totalCost);
  if (count == 0) {
    if (maxLen) out[0] = 0;
    return 0;
  }
  uint16_t n = buildMoves(gPathScratch, count, startHeading, out, maxLen);
  extendIntoGoal(maze, out, n, maxLen);
  return n;
}

// =============================================================================
//  Planner theo thoi gian: trang thai (o, huong, nuoc di o o truoc)
// =============================================================================
namespace {

enum Prev : uint8_t { P_F = 0, P_L = 1, P_R = 2 };

uint16_t msCost(float mm, float speed) {
  float t = mm / (speed > 1.0f ? speed : 1.0f) * 1000.0f;
  if (t < 1.0f) t = 1.0f;
  return uint16_t(t + 0.5f);
}

// Cost (ms) cua nuoc di `m` trong o hien tai, biet nuoc di o truoc la `prev`.
void fastCosts(const RunConfig& cfg, PathLanguage lang, uint16_t cost[3][3]) {
  const float h = cfg.halfCell;
  const float vTop = cfg.topSpeed;
  const TurnType t90 = lang >= LANG_SAFE ? TURN_F90 : TURN_SS90;
  const float v90 = cfg.turnSpeed[t90];
  const float vSS = cfg.turnSpeed[TURN_SS90];
  // thang
  cost[P_F][P_F] = msCost(2 * h, vTop);
  cost[P_L][P_F] = cost[P_R][P_F] = msCost(2 * h, 0.5f * (vTop + v90));
  // re sau doan thang
  cost[P_F][P_L] = cost[P_F][P_R] = msCost(cfg.turn[t90].length() * 0.5f + h, v90);
  if (lang == LANG_DIAGONALS) {
    // zig-zag = cheo
    cost[P_L][P_R] = cost[P_R][P_L] = msCost(1.41421356f * h, cfg.diagSpeed);
    // 2 lan re cung chieu = V90 / 180
    const float vV = cfg.turnSpeed[TURN_V90];
    cost[P_L][P_L] = cost[P_R][P_R] = msCost(cfg.turn[TURN_V90].length() * 0.5f + h, vV);
  } else {
    cost[P_L][P_R] = cost[P_R][P_L] = msCost(cfg.turn[TURN_SS90].length(), vSS);
    cost[P_L][P_L] = cost[P_R][P_R] = msCost(cfg.turn[TURN_SS90].length(), vSS);
  }
}

uint16_t planFast(const Maze& maze, const RunConfig& cfg, PathLanguage lang, char* out, uint16_t maxLen) {
  if (kFastStates == 0) return 0;
  const Location start{0, 0};
  if (!maze.isExit(start, NORTH, Maze::CLOSED)) return 0;  // o xuat phat phai mo phia Bac
  uint16_t cost[3][3];
  fastCosts(cfg, lang, cost);

  auto idx = [](Location c, Heading h, uint8_t p) { return uint16_t((c.index() * kHeadingCount + h) * 3 + p); };
  for (uint16_t i = 0; i < kFastStates; i++) {
    gDist[i] = kMaxCost;
    gPrev[i] = kNone;
  }
  gHeap.reset(kFastStates);
  const uint16_t s0 = idx(start.neighbour(NORTH), NORTH, P_F);  // sau 'B'
  gDist[s0] = 0;
  gHeap.push(s0);
  uint16_t goal = kNone;

  while (!gHeap.empty()) {
    const uint16_t u = gHeap.pop();
    const uint8_t p = u % 3;
    const Heading h = Heading((u / 3) % kHeadingCount);
    const Location c = Location::fromIndex(u / 3 / kHeadingCount);
    if (maze.isGoal(c)) {
      goal = u;
      break;
    }
    const uint32_t d = gDist[u];
    for (uint8_t m = 0; m < 3; m++) {
      const Heading nh = m == P_F ? h : (m == P_L ? leftOf(h) : rightOf(h));
      if (!maze.isExit(c, nh, Maze::CLOSED)) continue;
      relax(u, idx(c.neighbour(nh), nh, m), d + cost[p][m]);
    }
  }
  if (goal == kNone) return 0;

  // Truy vet: moi trang thai luu nuoc di cua o TRUOC no.
  static char rev[kCellCount + 4];
  uint16_t k = 0;
  for (uint16_t s = goal; s != s0 && s != kNone; s = gPrev[s]) {
    const uint8_t p = s % 3;
    rev[k++] = p == P_F ? 'F' : (p == P_L ? 'L' : 'R');
  }
  uint16_t n = 0;
  if (maxLen < k + 3) return 0;
  out[n++] = 'B';
  while (k) out[n++] = rev[--k];
  out[n++] = 'S';
  out[n] = 0;
  extendIntoGoal(maze, out, n, maxLen);
  return n;
}

}  // namespace

uint16_t buildFastRun(const Maze& maze, const RunConfig& cfg, PathLanguage maxLang, char* out, uint16_t maxLen,
                      SmoothMove* smooth, uint16_t maxSmooth, float* estTime, PathLanguage* used) {
  static char candidate[kCellCount + 8];
  float bestT = -1;
  // Ung vien: planner theo thoi gian cho tung ngon ngu + planner co ban (it re).
  for (int pass = 0; pass <= int(maxLang) + 1; pass++) {
    uint16_t n;
    if (pass <= int(maxLang)) {
      n = planFast(maze, cfg, PathLanguage(pass), candidate, sizeof(candidate));
    } else {
      PlannerCosts pc;
      pc.straight = 10;
      pc.turn = 15;
      n = buildSpeedRun(maze, NORTH, pc, candidate, sizeof(candidate));
    }
    if (!n) continue;
    static SmoothMove tmp[kMaxSmoothMoves];
    PathLanguage lang;
    float t;
    uint16_t m = buildSmoothPath(candidate, cfg, maxLang, tmp, sizeof(tmp) / sizeof(tmp[0]), &lang, &t);
    if (!m || t < 0) continue;
    if (bestT < 0 || t < bestT) {
      bestT = t;
      strncpy(out, candidate, maxLen);
      out[maxLen - 1] = 0;
      uint16_t c = m < maxSmooth - 1 ? m : uint16_t(maxSmooth - 1);
      memcpy(smooth, tmp, c * sizeof(SmoothMove));
      smooth[c] = SmoothMove{MK_END, TURN_SS90, 0};
      if (used) *used = lang;
    }
  }
  if (estTime) *estTime = bestT;
  if (bestT < 0) return 0;
  uint16_t n = 0;
  while (smooth[n].kind != MK_END) n++;
  return n;
}

bool isMazeSolved(const Maze& maze, Heading startHeading, const PlannerCosts& costs, Maze::Mask optimistic) {
  uint32_t known = 0;
  if (!planPath(maze, Location{0, 0}, startHeading, maze.goals(), maze.goalCount(), Maze::CLOSED, costs,
                gPathScratch, kCellCount, &known))
    return false;
  uint32_t optCost = 0;
  if (!planPath(maze, Location{0, 0}, startHeading, maze.goals(), maze.goalCount(), optimistic, costs,
                gPathScratch, kCellCount, &optCost))
    return false;
  return known <= optCost;
}

}  // namespace mm
