#include "maze.h"

#include <stdio.h>
#include <string.h>

namespace mm {

void Maze::reset() {
  memset(walls_, 0, sizeof(walls_));
  memset(visited_, 0, sizeof(visited_));
  for (uint16_t i = 0; i < kCellCount; i++) cost_[i] = kMaxCost;
  for (uint8_t i = 0; i < kMazeSize; i++) {
    applyWall(Location{i, 0}, SOUTH, true);
    applyWall(Location{i, uint8_t(kMazeSize - 1)}, NORTH, true);
    applyWall(Location{0, i}, WEST, true);
    applyWall(Location{uint8_t(kMazeSize - 1), i}, EAST, true);
  }
  setDefaultGoals();
  hasHint_ = false;
}

void Maze::setHint(const uint8_t* walls) {
  for (uint16_t i = 0; i < kCellCount; i++) hint_[i] = walls[i] & 0x0F;
  hasHint_ = true;
}

bool Maze::addGoal(Location g) {
  if (!g.isValid() || goalCount_ >= kMaxGoals || isGoal(g)) return false;
  goals_[goalCount_++] = g;
  return true;
}

void Maze::setDefaultGoals() {
  clearGoals();
  const uint8_t h = kMazeSize / 2;
  if (kMazeSize % 2 == 0) {
    addGoal(Location{uint8_t(h - 1), uint8_t(h - 1)});
    addGoal(Location{h, uint8_t(h - 1)});
    addGoal(Location{uint8_t(h - 1), h});
    addGoal(Location{h, h});
  } else {
    addGoal(Location{h, h});
  }
}

bool Maze::isGoal(Location c) const {
  for (uint8_t i = 0; i < goalCount_; i++)
    if (goals_[i] == c) return true;
  return false;
}

void Maze::applyWall(Location c, Heading h, bool present) {
  uint8_t& w = walls_[c.index()];
  w |= uint8_t(1u << (h + 4));
  if (present)
    w |= uint8_t(1u << h);
  else
    w &= uint8_t(~(1u << h));
}

void Maze::setWall(Location c, Heading h, bool present) {
  if (!c.isValid()) return;
  Location n = c.neighbour(h);
  if (!n.isValid()) return;  // tuong bao ngoai: luon co, khong cho ghi de
  applyWall(c, h, present);
  applyWall(n, behind(h), present);
}

bool Maze::isExit(Location c, Heading h, Mask mask) const {
  if (!c.neighbour(h).isValid()) return false;
  if (hasWall(c, h)) return false;
  if (isKnown(c, h)) return true;
  if (mask == CLOSED) return false;
  if (mask == HINT && hasHint_ && hintWall(c, h)) return false;
  return true;
}

void Maze::flood(const Location* targets, uint8_t count, Mask mask) {
  static uint16_t queue[kCellCount];  // BFS: moi o vao hang doi toi da 1 lan
  for (uint16_t i = 0; i < kCellCount; i++) cost_[i] = kMaxCost;

  uint16_t head = 0, tail = 0;
  for (uint8_t i = 0; i < count; i++) {
    if (!targets[i].isValid()) continue;
    uint16_t idx = targets[i].index();
    if (cost_[idx] == 0) continue;
    cost_[idx] = 0;
    queue[tail++] = idx;
  }

  while (head < tail) {
    uint16_t idx = queue[head++];
    Location c = Location::fromIndex(idx);
    uint16_t next = uint16_t(cost_[idx] + 1);
    for (uint8_t h = 0; h < kHeadingCount; h++) {
      if (!isExit(c, Heading(h), mask)) continue;
      uint16_t n = c.neighbour(Heading(h)).index();
      if (cost_[n] > next) {
        cost_[n] = next;
        queue[tail++] = n;
      }
    }
  }
}

bool Maze::bestHeading(Location c, Heading current, Mask mask, Heading& out) const {
  static const Turn kOrder[4] = {AHEAD, RIGHT, LEFT, BACK};
  uint16_t best = cost_[c.index()];
  bool found = false;
  for (uint8_t i = 0; i < 4; i++) {
    Heading h = applyTurn(current, kOrder[i]);
    if (!isExit(c, h, mask)) continue;
    uint16_t nc = cost_[c.neighbour(h).index()];
    if (nc < best) {
      best = nc;
      out = h;
      found = true;
    }
  }
  return found;
}

uint16_t Maze::checksum() const {
  // CRC16-CCITT tren mang tuong
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < kCellCount; i++) {
    crc ^= uint16_t(walls_[i]) << 8;
    for (uint8_t b = 0; b < 8; b++) crc = (crc & 0x8000) ? uint16_t((crc << 1) ^ 0x1021) : uint16_t(crc << 1);
  }
  return crc;
}

void Maze::print(void (*emit)(const char* line), bool showCost) const {
  // Dinh dang giong micromouseonline/mazefiles: 'o' = cot, '---' / '|' = tuong.
  // Tuong chua biet: ' . ' (ngang) va ':' (doc).
  static char line[kMazeSize * 4 + 2];
  for (int y = kMazeSize - 1; y >= 0; y--) {
    // Hang tuong phia bac
    int p = 0;
    for (uint8_t x = 0; x < kMazeSize; x++) {
      Location c{x, uint8_t(y)};
      line[p++] = 'o';
      const char* s = hasWall(c, NORTH) ? "---" : (isKnown(c, NORTH) ? "   " : " . ");
      memcpy(line + p, s, 3);
      p += 3;
    }
    line[p++] = 'o';
    line[p] = 0;
    emit(line);
    // Hang o
    p = 0;
    for (uint8_t x = 0; x < kMazeSize; x++) {
      Location c{x, uint8_t(y)};
      line[p++] = hasWall(c, WEST) ? '|' : (isKnown(c, WEST) ? ' ' : ':');
      char cell[4] = "   ";
      if (showCost && cost_[c.index()] != kMaxCost) {
        snprintf(cell, sizeof(cell), "%3u", unsigned(cost_[c.index()] % 1000));
      } else if (isGoal(c)) {
        cell[1] = 'G';
      } else if (x == 0 && y == 0) {
        cell[1] = 'S';
      } else if (isVisited(c)) {
        cell[1] = '.';
      }
      memcpy(line + p, cell, 3);
      p += 3;
    }
    Location last{uint8_t(kMazeSize - 1), uint8_t(y)};
    line[p++] = hasWall(last, EAST) ? '|' : ':';
    line[p] = 0;
    emit(line);
  }
  int p = 0;
  for (uint8_t x = 0; x < kMazeSize; x++) {
    memcpy(line + p, "o---", 4);
    p += 4;
  }
  line[p++] = 'o';
  line[p] = 0;
  emit(line);
}

}  // namespace mm
