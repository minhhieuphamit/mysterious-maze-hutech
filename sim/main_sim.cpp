// =============================================================================
//  Adapter cho bo mo phong mms (https://github.com/mackorone/mms, MIT)
//
//  mms chay chuong trinh nay nhu 1 tien trinh con: lenh gui qua stdout,
//  phan hoi doc tu stdin. Log ghi ra stderr (hien trong khung log cua mms).
//
//  Kich ban giong ngay thi dau:
//    Luot 1: tham do (0,0) -> dich, tiep tuc tham do roi quay ve (0,0)
//    Luot 2: chay toc do theo duong ngan nhat da biet
// =============================================================================
#include <iostream>
#include <string>

#include "explorer.h"
#include "maze.h"
#include "planner.h"

using namespace mm;

namespace api {

std::string send(const std::string& cmd, bool reply) {
  std::cout << cmd << std::endl;
  if (!reply) return "";
  std::string r;
  std::getline(std::cin, r);
  return r;
}
int mazeWidth() { return std::stoi(send("mazeWidth", true)); }
int mazeHeight() { return std::stoi(send("mazeHeight", true)); }
bool wallFront() { return send("wallFront", true) == "true"; }
bool wallLeft() { return send("wallLeft", true) == "true"; }
bool wallRight() { return send("wallRight", true) == "true"; }
bool moveForward(int n = 1) { return send("moveForward " + std::to_string(n), true) == "ack"; }
void turnLeft() { send("turnLeft", true); }
void turnRight() { send("turnRight", true); }
void setWall(int x, int y, char d) { send("setWall " + std::to_string(x) + " " + std::to_string(y) + " " + d, false); }
void setColor(int x, int y, char c) { send("setColor " + std::to_string(x) + " " + std::to_string(y) + " " + c, false); }
void clearAllColor() { send("clearAllColor", false); }
void setText(int x, int y, const std::string& t) {
  send("setText " + std::to_string(x) + " " + std::to_string(y) + " " + t, false);
}
void log(const std::string& s) { std::cerr << s << std::endl; }

}  // namespace api

class SimIO : public MouseIO {
 public:
  void senseWalls(bool& l, bool& f, bool& r) override {
    l = api::wallLeft();
    f = api::wallFront();
    r = api::wallRight();
  }
  bool move(Turn t) override {
    switch (t) {
      case LEFT: api::turnLeft(); break;
      case RIGHT: api::turnRight(); break;
      case BACK:
        api::turnRight();
        api::turnRight();
        break;
      default: break;
    }
    return api::moveForward();
  }
  void stopAtCenter() override {}

  void onCellUpdated(const Maze& maze, Location c, Heading) override {
    // Ve tuong da biet cua o hien tai
    for (uint8_t h = 0; h < 4; h++)
      if (maze.hasWall(c, Heading(h))) api::setWall(c.x, c.y, headingChar(Heading(h)));
    // Chi gui lai text cua o co cost thay doi (giam so lenh)
    for (uint16_t i = 0; i < kCellCount; i++) {
      Location p = Location::fromIndex(i);
      uint16_t v = maze.cost(p);
      if (v != shown_[i]) {
        shown_[i] = v;
        api::setText(p.x, p.y, v == kMaxCost ? "-" : std::to_string(v));
      }
    }
    api::setColor(c.x, c.y, 'c');
  }

 private:
  uint16_t shown_[kCellCount] = {};
};

static void turnTo(Heading& cur, Heading want) {
  while (cur != want) {
    api::turnRight();
    cur = rightOf(cur);
  }
}

int main() {
  if (api::mazeWidth() != kMazeSize || api::mazeHeight() != kMazeSize) {
    api::log("Kich thuoc me cung khac MAZE_SIZE luc build! Build lai voi -DMAZE_SIZE=N.");
    return 1;
  }
  Maze maze;
  SimIO io;
  Explorer ex(maze, io);
  PlannerCosts costs;  // mac dinh: re tai cho

  for (uint8_t i = 0; i < maze.goalCount(); i++) api::setColor(maze.goals()[i].x, maze.goals()[i].y, 'G');

  api::log("== Luot 1: tham do toi dich ==");
  ex.setPose(Location{0, 0}, NORTH);
  if (ex.searchToGoals() != SearchResult::REACHED) {
    api::log("Khong tim thay duong toi dich!");
    return 1;
  }
  api::log("Toi dich sau " + std::to_string(ex.steps()) + " buoc. Tiep tuc tham do + ve start...");
  bool solved = ex.exploreUntilSolved(costs, 300);
  api::log(std::string("Ban do ") + (solved ? "DA" : "CHUA") + " toi uu. Tong buoc tham do: " +
           std::to_string(ex.steps()));

  // Dat lai robot o start, huong bac (giong nhu nguoi dieu khien dat lai robot)
  Heading h = ex.heading();
  turnTo(h, NORTH);

  char moves[kCellCount * 2];
  uint32_t cost = 0;
  if (!buildSpeedRun(maze, NORTH, costs, moves, sizeof(moves), &cost)) {
    api::log("Khong co duong da biet toi dich!");
    return 1;
  }
  api::log("== Luot 2: speed run. Lenh: " + std::string(moves) + "  cost=" + std::to_string(cost));

  // To mau duong chay
  api::clearAllColor();
  {
    Location p{0, 0};
    Heading d = NORTH;
    api::setColor(p.x, p.y, 'Y');
    for (const char* c = moves; *c; c++) {
      if (*c == 'l') d = leftOf(d);
      if (*c == 'r') d = rightOf(d);
      if (*c == 'a') d = behind(d);
      if (*c == 'L') d = leftOf(d);
      if (*c == 'R') d = rightOf(d);
      if (*c == 'B' || *c == 'F' || *c == 'L' || *c == 'R') {
        p = p.neighbour(d);
        api::setColor(p.x, p.y, 'Y');
      }
    }
  }

  // Thuc thi: gop cac doan thang thanh moveForward(n)
  int pending = 0;
  auto flush = [&]() {
    if (pending > 0 && !api::moveForward(pending)) api::log("CRASH!");
    pending = 0;
  };
  for (const char* c = moves; *c; c++) {
    switch (*c) {
      case 'l': api::turnLeft(); break;
      case 'r': api::turnRight(); break;
      case 'a': api::turnRight(); api::turnRight(); break;
      case 'B':
      case 'F': pending++; break;
      case 'L': flush(); api::turnLeft(); pending = 1; break;
      case 'R': flush(); api::turnRight(); pending = 1; break;
      case 'S': flush(); break;
    }
  }
  flush();
  api::log("== Speed run xong ==");
  return 0;
}
