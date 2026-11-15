// =============================================================================
//  Explorer - thuat toan tham do me cung (Flood Fill) doc lap phan cung.
//
//  Cung 1 doan code nay chay tren:
//    - robot that  (RobotIO trong firmware)
//    - bo mo phong mms (SimIO trong sim/)
//    - test tu dong tren may tinh (FakeIO trong test/)
// =============================================================================
#pragma once
#include "maze.h"
#include "planner.h"

namespace mm {

// Giao dien phan cung ma Explorer can. Moi nen tang tu cai dat.
class MouseIO {
 public:
  virtual ~MouseIO() {}
  // Doc tuong cua O HIEN TAI (trai / truoc / phai so voi huong cua chuot).
  virtual void senseWalls(bool& left, bool& front, bool& right) = 0;
  // Thuc hien re t roi di vao o ke ben, dung lai tai "diem doc cam bien" cua o moi.
  // Tra ve false neu bi huy (va cham, nut bam...).
  virtual bool move(Turn t) = 0;
  // Da den noi: dung lai o tam o hien tai.
  virtual void stopAtCenter() = 0;
  virtual bool shouldAbort() { return false; }
  // Hook hien thi / debug (mms ve tuong & cost).
  virtual void onCellUpdated(const Maze&, Location, Heading) {}
};

enum class SearchResult : uint8_t { REACHED, NO_ROUTE, ABORTED };

class Explorer {
 public:
  Explorer(Maze& maze, MouseIO& io) : maze_(maze), io_(io) {}

  void setPose(Location l, Heading h) {
    loc_ = l;
    heading_ = h;
  }
  Location location() const { return loc_; }
  Heading heading() const { return heading_; }
  uint16_t steps() const { return steps_; }

  // Tham do toi vung dich (4 o trung tam) va dung o tam o dich.
  SearchResult searchToGoals() { return search(maze_.goals(), maze_.goalCount(), true); }
  // Tham do toi 1 o bat ky (vd. quay ve (0,0)) va dung lai.
  SearchResult searchTo(Location target) { return search(&target, 1, true); }

  // Tham do cac o CHUA DI QUA nam tren duong lac quan tot nhat (start->dich) cho toi khi
  // duong da biet = duong toi uu, hoac da di them maxCells o. Chuot khong dung lai giua
  // cac chang. Cuoi cung quay ve start va dung (tru khi bi huy / het gio).
  // Tra ve true neu ban do da "giai xong".
  bool exploreUntilSolved(const PlannerCosts& costs, uint16_t maxCells);

  // ---- Ban do goi y (maze.setHint) ----
  // So tuong do duoc KHAC voi goi y. Vuot nguong -> bo goi y, tham do binh thuong.
  uint16_t hintMismatches() const { return mismatches_; }
  bool hintRejected() const { return hintRejected_; }
  void setHintTolerance(uint16_t n) { hintTolerance_ = n; }

 private:
  SearchResult search(const Location* targets, uint8_t count, bool stopAtEnd);
  void updateWalls();
  void sense(Heading h, bool wall);
  Maze::Mask exploreMask() const { return maze_.hasHint() ? Maze::HINT : Maze::OPEN; }

  Maze& maze_;
  MouseIO& io_;
  Location loc_{0, 0};
  Heading heading_ = NORTH;
  uint16_t steps_ = 0;
  uint16_t mismatches_ = 0;
  uint16_t hintTolerance_ = 4;
  bool hintRejected_ = false;
};

}  // namespace mm
