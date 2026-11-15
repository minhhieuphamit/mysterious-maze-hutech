// Sinh me cung ngau nhien + doc file mazefiles (dung chung cho cac test host).
#pragma once
#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "maze.h"

namespace mm {

// ---------------------------------------------------------------- Me cung that
// Dung chinh class Maze lam "me cung that": moi tuong deu da biet.
inline void fillAllWalls(Maze& m) {
  m.reset();
  for (uint8_t x = 0; x < kMazeSize; x++)
    for (uint8_t y = 0; y < kMazeSize; y++)
      for (uint8_t h = 0; h < 4; h++) m.setWall(Location{x, y}, Heading(h), true);
}

// DFS sinh me cung hoan hao, sau do pha them mot so tuong de tao vong lap.
inline void generateMaze(Maze& m, uint32_t seed, double loopRatio) {
  std::mt19937 rng(seed);
  fillAllWalls(m);
  std::vector<bool> seen(kCellCount, false);
  std::vector<Location> stack{{0, 0}};
  seen[0] = true;
  while (!stack.empty()) {
    Location c = stack.back();
    Heading opts[4];
    int n = 0;
    for (uint8_t h = 0; h < 4; h++) {
      Location nb = c.neighbour(Heading(h));
      if (nb.isValid() && !seen[nb.index()]) opts[n++] = Heading(h);
    }
    if (n == 0) {
      stack.pop_back();
      continue;
    }
    Heading h = opts[rng() % n];
    Location nb = c.neighbour(h);
    m.setWall(c, h, false);
    seen[nb.index()] = true;
    stack.push_back(nb);
  }
  // Vong lap
  std::uniform_real_distribution<double> u(0, 1);
  for (uint8_t x = 0; x < kMazeSize; x++)
    for (uint8_t y = 0; y < kMazeSize; y++)
      for (Heading h : {NORTH, EAST})
        if (u(rng) < loopRatio) m.setWall(Location{x, y}, h, false);

  // Vung dich 2x2: mo ben trong, bao quanh bang tuong, chua 1 loi vao.
  const uint8_t a = kMazeSize / 2 - 1, b = kMazeSize / 2;
  m.setWall({a, a}, EAST, false);
  m.setWall({a, b}, EAST, false);
  m.setWall({a, a}, NORTH, false);
  m.setWall({b, a}, NORTH, false);
  for (uint8_t i = a; i <= b; i++) {
    m.setWall({i, a}, SOUTH, true);
    m.setWall({i, b}, NORTH, true);
    m.setWall({a, i}, WEST, true);
    m.setWall({b, i}, EAST, true);
  }
  const Location doors[8] = {{a, a}, {b, a}, {a, b}, {b, b}, {a, a}, {a, b}, {b, a}, {b, b}};
  const Heading doorH[8] = {SOUTH, SOUTH, NORTH, NORTH, WEST, WEST, EAST, EAST};
  int d = rng() % 8;
  m.setWall(doors[d], doorH[d], false);

  // O xuat phat: 3 mat tuong, chi mo phia bac (luat quoc te).
  m.setWall({0, 0}, EAST, true);
  m.setWall({0, 0}, NORTH, false);
}

// Moi o deu di toi duoc tu (0,0)?
inline bool fullyConnected(Maze& m) {
  m.floodTo({0, 0}, Maze::CLOSED);
  for (uint16_t i = 0; i < kCellCount; i++)
    if (m.cost(Location::fromIndex(i)) == kMaxCost) return false;
  return true;
}

inline void generateConnectedMaze(Maze& m, uint32_t seed, double loopRatio) {
  for (uint32_t k = 0;; k++) {
    generateMaze(m, seed + k * 100003u, loopRatio);
    if (fullyConnected(m)) return;
  }
}

// Doc file dinh dang mazefiles: 'o' cot, '---' tuong ngang, '|' tuong doc.
inline bool loadMazeFile(const std::string& path, Maze& m) {
  std::ifstream f(path);
  if (!f) return false;
  std::vector<std::string> lines;
  std::string s;
  while (std::getline(f, s)) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
    if (!s.empty()) lines.push_back(s);
  }
  int size = int(lines.size() / 2);
  if (size != kMazeSize) {
    std::printf("  File %s co kich thuoc %d, can %d\n", path.c_str(), size, kMazeSize);
    return false;
  }
  m.reset();
  for (int y = 0; y < size; y++) {
    const std::string& top = lines[lines.size() - 1 - (2 * y + 2)];
    const std::string& mid = lines[lines.size() - 1 - (2 * y + 1)];
    for (int x = 0; x < size; x++) {
      Location c{uint8_t(x), uint8_t(y)};
      size_t ti = 4 * x + 2, wi = 4 * x, ei = 4 * x + 4;
      m.setWall(c, NORTH, ti < top.size() && top[ti] == '-');
      m.setWall(c, WEST, wi < mid.size() && mid[wi] == '|');
      m.setWall(c, EAST, ei < mid.size() && mid[ei] == '|');
      // SOUTH duoc set boi hang duoi (hoac tuong bao)
    }
  }
  // Dich: o co ky tu 'G' (neu file co), nguoc lai giu 2x2 mac dinh.
  std::vector<Location> goals;
  for (int y = 0; y < size; y++) {
    const std::string& mid = lines[lines.size() - 1 - (2 * y + 1)];
    for (int x = 0; x < size; x++)
      if (size_t(4 * x + 2) < mid.size() && mid[4 * x + 2] == 'G') goals.push_back({uint8_t(x), uint8_t(y)});
  }
  if (!goals.empty()) {
    m.clearGoals();
    for (auto g : goals) m.addGoal(g);
  }
  return true;
}


}  // namespace mm
