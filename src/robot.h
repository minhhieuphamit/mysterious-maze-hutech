// =============================================================================
//  Robot - noi thuat toan (MazeCore) voi chuyen dong that.
// =============================================================================
#pragma once
#include "config.h"
#include "explorer.h"
#include "smooth_path.h"

namespace robot {

// Thong so re (before/after, ban kinh...) tinh 1 lan luc khoi dong.
bool initTurns();
const mm::RunConfig& runConfig(uint8_t index);  // RUN_PARAMS[index]
const mm::TurnParams& searchTurn();             // SS90 dung khi tham do
bool smoothSearchAvailable();

// Cai dat MouseIO cho robot that (he quy chieu "canh o", xem docs/05-thuat-toan.md)
class RobotIO : public mm::MouseIO {
 public:
  enum Pose : uint8_t { AT_START, AT_CENTER, AT_SENSING };

  explicit RobotIO(const SearchParams& p) : params_(p), startMs_(millis()) {}
  // Gioi han thoi gian tinh tu luc goi (ngay khi xuat phat). 0 = khong gioi han.
  void setDeadline(uint32_t ms) {
    startMs_ = millis();
    deadlineMs_ = ms;
  }
  bool timeUp() const { return deadlineMs_ && millis() - startMs_ > deadlineMs_; }
  void senseWalls(bool& left, bool& front, bool& right) override;
  bool move(mm::Turn t) override;
  void stopAtCenter() override;
  bool shouldAbort() override;

 private:
  const SearchParams& params_;
  Pose pose_ = AT_START;
  bool lastFrontWall_ = false;
  uint32_t startMs_ = 0;
  uint32_t deadlineMs_ = 0;
};

// Chuyen dong co ban (tra ve false neu bi huy / va cham)
bool waitForward();
bool waitTurn();
bool waitCurve();
bool turnInPlace(mm::Turn t, float omega, float alpha);
bool straight(float distance, float topSpeed, float finalSpeed, float accel, bool steering = true);
void alignToFrontWall();

// Chay chuoi chuyen dong slalom da dich (buildFastRun)
bool runSmooth(const mm::SmoothMove* moves, const mm::RunConfig& cfg);

// Cho "vay tay" truoc cam bien truoc de xuat phat
void waitForHandStart();

}  // namespace robot
