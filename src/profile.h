// =============================================================================
//  Profile - bien dang van toc hinh thang (tang toc -> chay deu -> giam toc)
//
//  Vi tri la TICH LUY: extend() cong them quang duong vao dich truoc do, nen cac
//  doan chuyen dong noi tiep nhau khong bi sai so (ke ca khi CPU cham quyet dinh
//  va robot "troi" them vai mm o toc do cuoi).
//  Duoc cap nhat trong ngat dieu khien; ham goi tu main phai tat ngat (Motion lo).
// =============================================================================
#pragma once
#include <math.h>
#include <stdint.h>

class Profile {
 public:
  enum State : uint8_t { IDLE, RUNNING, FINISHED, MANUAL };

  void reset() {
    state_ = IDLE;
    pos_ = target_ = speed_ = accelNow_ = delta_ = 0;
  }

  // Them 1 doan `distance` (co dau) tinh tu dich cu. Giu nguyen van toc hien tai.
  void extend(float distance, float topSpeed, float finalSpeed, float accel) {
    target_ += distance;
    float remaining = target_ - pos_;
    sign_ = remaining >= 0 ? 1.0f : -1.0f;
    top_ = fabsf(topSpeed);
    final_ = fabsf(finalSpeed);
    accel_ = fabsf(accel);
    state_ = RUNNING;
  }

  // Lay vi tri hien tai lam moc moi (sau khi re cong, can chinh tuong...).
  void rebase() { target_ = pos_; }
  // Dich moc di 1 doan chinh xac (vd. do dai duong cong vua chay) - khong mat sai so.
  void shiftTarget(float d) { target_ += d; }
  float target() const { return target_; }

  // Dieu khien truc tiep van toc (dung khi can chinh theo cam bien).
  void setSpeed(float v) {
    speed_ = v;
    state_ = MANUAL;
  }
  void stop() {
    speed_ = 0;
    target_ = pos_;
    state_ = FINISHED;
  }

  void update(float dt) {
    const float before = pos_;
    const float oldSpeed = speed_;
    if (state_ == RUNNING) {
      const float remaining = (target_ - pos_) * sign_;
      float v = speed_ * sign_;  // van toc theo chieu chuyen dong
      const float brakeDist = (v * v - final_ * final_) / (2.0f * accel_);
      const float wanted = (remaining <= brakeDist) ? final_ : top_;
      if (v < wanted)
        v = fminf(v + accel_ * dt, wanted);
      else if (v > wanted)
        v = fmaxf(v - accel_ * dt, wanted);
      // Dam bao toi dich khi final = 0 (bo qua sai so roi rac)
      if (v < kCreepSpeed && remaining > 0) v = kCreepSpeed;
      const float step = v * dt;
      if (step >= remaining) {
        pos_ = target_;
        speed_ = final_ * sign_;
        state_ = FINISHED;
      } else {
        pos_ += step * sign_;
        speed_ = v * sign_;
      }
    } else if (state_ == FINISHED || state_ == MANUAL) {
      pos_ += speed_ * dt;  // FINISHED voi final > 0: tiep tuc chay deu
    }
    delta_ = pos_ - before;
    accelNow_ = (speed_ - oldSpeed) / dt;
  }

  bool isFinished() const { return state_ != RUNNING; }
  float position() const { return pos_; }
  float speed() const { return speed_; }
  float acceleration() const { return accelNow_; }
  float delta() const { return delta_; }  // quang duong cua chu ky vua roi

 private:
  static constexpr float kCreepSpeed = 15.0f;
  volatile State state_ = IDLE;
  float pos_ = 0, target_ = 0, speed_ = 0, accelNow_ = 0, delta_ = 0;
  float sign_ = 1, top_ = 0, final_ = 0, accel_ = 1;
};
