#include "explorer.h"

namespace mm {

namespace {
Location gTargets[kCellCount];
Location gPath[kCellCount];

bool contains(const Location* list, uint16_t n, Location c) {
  for (uint16_t i = 0; i < n; i++)
    if (list[i] == c) return true;
  return false;
}
}  // namespace

void Explorer::sense(Heading h, bool wall) {
  // Doi chieu voi ban do goi y (chi tinh lan dau do tuong do)
  if (maze_.hasHint() && !maze_.isKnown(loc_, h) && maze_.hintWall(loc_, h) != wall) {
    mismatches_++;
    if (mismatches_ > hintTolerance_) {
      maze_.clearHint();  // goi y sai qua nhieu -> co the la me cung khac: bo, tham do binh thuong
      hintRejected_ = true;
    }
  }
  maze_.setWall(loc_, h, wall);
}

void Explorer::updateWalls() {
  bool left = false, front = false, right = false;
  io_.senseWalls(left, front, right);
  sense(leftOf(heading_), left);
  sense(heading_, front);
  sense(rightOf(heading_), right);
  // Vua di qua canh phia sau -> chac chan khong co tuong.
  if (steps_ > 0) sense(behind(heading_), false);
  maze_.markVisited(loc_);
}

SearchResult Explorer::search(const Location* targets, uint8_t count, bool stopAtEnd) {
  for (;;) {
    if (io_.shouldAbort()) return SearchResult::ABORTED;
    updateWalls();
    maze_.flood(targets, count, exploreMask());
    io_.onCellUpdated(maze_, loc_, heading_);
    if (contains(targets, count, loc_)) {
      if (stopAtEnd) io_.stopAtCenter();
      return SearchResult::REACHED;
    }
    Heading next;
    if (!maze_.bestHeading(loc_, heading_, exploreMask(), next)) {
      if (maze_.hasHint()) {  // goi y chan het duong -> goi y sai, bo di va tinh lai
        maze_.clearHint();
        hintRejected_ = true;
        continue;
      }
      if (stopAtEnd) io_.stopAtCenter();
      return SearchResult::NO_ROUTE;
    }
    if (!io_.move(turnBetween(heading_, next))) return SearchResult::ABORTED;
    heading_ = next;
    loc_ = loc_.neighbour(next);
    steps_++;
  }
}

bool Explorer::exploreUntilSolved(const PlannerCosts& costs, uint16_t maxCells) {
  const Location start{0, 0};
  const uint16_t startSteps = steps_;
  bool solved = false;
  // Gioi han so lan tinh lai muc tieu de tranh vong lap vo han khi muc tieu khong toi duoc
  for (uint16_t leg = 0; leg < 4 * kCellCount; leg++) {
    if (uint16_t(steps_ - startSteps) >= maxCells) break;
    if (isMazeSolved(maze_, NORTH, costs, exploreMask())) {
      solved = true;
      break;
    }
    // Muc tieu = cac o chua tham tren duong lac quan tot nhat start -> dich.
    uint16_t n = planPath(maze_, start, NORTH, maze_.goals(), maze_.goalCount(), exploreMask(), costs, gPath,
                          kCellCount);
    uint8_t m = 0;
    for (uint16_t i = 0; i < n && m < 255; i++)
      if (!maze_.isVisited(gPath[i]) && !contains(gTargets, m, gPath[i])) gTargets[m++] = gPath[i];
    if (m == 0) {
      solved = true;
      break;
    }
    // Flood tu nhieu dich -> chuot di toi o gan nhat, KHONG dung lai.
    if (search(gTargets, m, false) == SearchResult::ABORTED) return false;
  }
  if (!solved) solved = isMazeSolved(maze_, NORTH, costs, exploreMask());
  if (searchTo(start) == SearchResult::ABORTED) return false;
  return solved;
}

}  // namespace mm
