// =============================================================================
//  MazeCore - kieu du lieu co ban cho Micromouse (doc lap phan cung)
//  Dung chung cho: firmware STM32, bo mo phong mms, test tren may tinh.
// =============================================================================
#pragma once
#include <stdint.h>

#ifndef MAZE_SIZE
#define MAZE_SIZE 16  // BTC: 16 x 16 o (du kien). Doi bang -DMAZE_SIZE=N neu can.
#endif

namespace mm {

constexpr uint8_t kMazeSize = MAZE_SIZE;
constexpr uint16_t kCellCount = uint16_t(kMazeSize) * kMazeSize;
constexpr uint16_t kMaxCost = 0xFFFF;

// Huong tuyet doi. Quy uoc: o (0,0) o goc duoi-trai, NORTH = +y, EAST = +x.
enum Heading : uint8_t { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };
constexpr uint8_t kHeadingCount = 4;

inline Heading rightOf(Heading h) { return Heading((h + 1) & 3); }
inline Heading leftOf(Heading h) { return Heading((h + 3) & 3); }
inline Heading behind(Heading h) { return Heading((h + 2) & 3); }
inline char headingChar(Heading h) { return "nesw"[h]; }

// Huong tuong doi so voi huong hien tai cua chuot.
enum Turn : uint8_t { AHEAD = 0, RIGHT = 1, BACK = 2, LEFT = 3 };
inline Turn turnBetween(Heading from, Heading to) { return Turn((to - from + 4) & 3); }
inline Heading applyTurn(Heading h, Turn t) { return Heading((h + t) & 3); }

struct Location {
  uint8_t x;
  uint8_t y;

  bool operator==(const Location& o) const { return x == o.x && y == o.y; }
  bool operator!=(const Location& o) const { return !(*this == o); }
  bool isValid() const { return x < kMazeSize && y < kMazeSize; }
  uint16_t index() const { return uint16_t(y) * kMazeSize + x; }

  static Location fromIndex(uint16_t i) {
    return Location{uint8_t(i % kMazeSize), uint8_t(i / kMazeSize)};
  }

  // O ke ben theo huong h. Neu ra ngoai me cung thi ket qua isValid() == false
  // (uint8_t 0 - 1 = 255).
  Location neighbour(Heading h) const {
    switch (h) {
      case NORTH: return Location{x, uint8_t(y + 1)};
      case EAST: return Location{uint8_t(x + 1), y};
      case SOUTH: return Location{x, uint8_t(y - 1)};
      default: return Location{uint8_t(x - 1), y};
    }
  }
};

// Huong di tu o a sang o ke ben b (a, b phai ke nhau).
inline Heading headingBetween(Location a, Location b) {
  if (b.x > a.x) return EAST;
  if (b.x < a.x) return WEST;
  if (b.y > a.y) return NORTH;
  return SOUTH;
}

}  // namespace mm
