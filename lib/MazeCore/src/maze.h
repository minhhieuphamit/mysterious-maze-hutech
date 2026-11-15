// =============================================================================
//  Maze - ban do tuong + Flood Fill
//
//  Moi o luu 1 byte:  bit 0..3 = co tuong (N,E,S,W)
//                     bit 4..7 = tuong do da duoc biet (da quet bang cam bien)
//  Tuong giua 2 o luon duoc cap nhat dong thoi o ca 2 phia.
// =============================================================================
#pragma once
#include "maze_types.h"

namespace mm {

class Maze {
 public:
  // Cach xu ly tuong CHUA BIET khi tinh duong:
  //  OPEN   : coi nhu khong co tuong  (lac quan - dung khi tham do)
  //  CLOSED : coi nhu co tuong        (than trong - dung cho luot chay toc do)
  //  HINT   : lay theo ban do goi y nap san (neu co), khong co goi y thi nhu OPEN
  enum Mask : uint8_t { OPEN = 0, CLOSED = 1, HINT = 2 };

  static constexpr uint8_t kMaxGoals = 8;

  Maze() { reset(); }

  // Xoa ban do: moi tuong ben trong la "chua biet", tuong bao quanh = co tuong. Xoa ca goi y.
  void reset();

  // ---- Ban do goi y (nap san, CHUA kiem chung) ----
  // walls[i]: bit0..3 = tuong N,E,S,W cua o index i (= y*kMazeSize + x).
  // Goi y chi dung de CHON HUONG khi tham do (mask HINT); speed run chi dung tuong da do.
  void setHint(const uint8_t* walls);
  void clearHint() { hasHint_ = false; }
  bool hasHint() const { return hasHint_; }
  bool hintWall(Location c, Heading h) const { return hint_[c.index()] & (1u << h); }

  // ---- Dich (Goal) ----
  void clearGoals() { goalCount_ = 0; }
  bool addGoal(Location g);
  void setDefaultGoals();  // 2x2 o trung tam (BTC: 4 o "G")
  bool isGoal(Location c) const;
  uint8_t goalCount() const { return goalCount_; }
  const Location* goals() const { return goals_; }

  // ---- Tuong ----
  void setWall(Location c, Heading h, bool present);
  bool hasWall(Location c, Heading h) const { return walls_[c.index()] & (1u << h); }
  bool isKnown(Location c, Heading h) const { return walls_[c.index()] & (1u << (h + 4)); }
  bool isCellFullyKnown(Location c) const { return (walls_[c.index()] & 0xF0) == 0xF0; }
  // Co the di tu c sang o ke ben theo huong h khong (tuy theo mask)?
  bool isExit(Location c, Heading h, Mask mask) const;

  void markVisited(Location c) { visited_[c.index() >> 3] |= uint8_t(1u << (c.index() & 7)); }
  bool isVisited(Location c) const { return visited_[c.index() >> 3] & (1u << (c.index() & 7)); }

  // ---- Flood Fill (BFS, moi buoc = 1 o) ----
  void flood(const Location* targets, uint8_t count, Mask mask);
  void floodToGoals(Mask mask) { flood(goals_, goalCount_, mask); }
  void floodTo(Location target, Mask mask) { flood(&target, 1, mask); }
  uint16_t cost(Location c) const { return cost_[c.index()]; }

  // Chon huong tot nhat (o ke ben co cost nho nhat). Uu tien: thang > phai > trai > quay dau.
  // Tra ve false neu khong co duong.
  bool bestHeading(Location c, Heading current, Mask mask, Heading& out) const;

  // ---- Luu / nap (Flash) ----
  uint8_t rawCell(uint16_t i) const { return walls_[i]; }
  void setRawCell(uint16_t i, uint8_t v) { walls_[i] = v; }
  uint16_t checksum() const;

  // In ban do ra dang ASCII (moi dong goi emit 1 lan). showCost = in cost flood fill.
  void print(void (*emit)(const char* line), bool showCost = false) const;

 private:
  void applyWall(Location c, Heading h, bool present);

  uint8_t walls_[kCellCount];
  uint8_t visited_[(kCellCount + 7) / 8];
  uint16_t cost_[kCellCount];
  Location goals_[kMaxGoals];
  uint8_t goalCount_ = 0;
  uint8_t hint_[kCellCount];
  bool hasHint_ = false;
};

}  // namespace mm
