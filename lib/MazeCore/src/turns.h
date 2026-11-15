// =============================================================================
//  Turns - hinh hoc cac kieu re toc do cao (slalom) cho me cung buoc o 192 mm
//
//  Y tuong lay tu Bulebule/mmlib (GPL-3.0, https://github.com/Bulebots/mmlib):
//    - Do cong (curvature) thay doi theo QUANG DUONG: tang hinh sin trong doan
//      "transition", giu khong doi trong doan "arc", giam hinh sin -> robot khong
//      bi giat goc, khong phu thuoc toc do thuc.
//    - Moi kieu re co "before" / "after": doan thang truoc/sau duong cong so voi
//      diem moc (trung diem canh o). Co the am (an vao doan thang ke ben).
//  Khac Bulebule: before/after duoc TINH TU DONG bang tich phan so (Bulebule
//  dung bang so do tay cho o 180 mm), nen doi ban kinh / kich thuoc o la xong.
//
//  Quy uoc: phien ban re TRAI (goc duong). Re phai = doi dau truc y.
// =============================================================================
#pragma once
#include <stdint.h>

namespace mm {

enum TurnType : uint8_t {
  TURN_SS90 = 0,  // re 90 trong 1 o, ban kinh nho (tham do + zig-zag khong cheo)
  TURN_F90,       // re 90 ban kinh lon, an vao 2 doan thang ke ben
  TURN_F180,      // quay dau 180 qua 2 o
  TURN_D45_IN,    // thang -> cheo 45
  TURN_D45_OUT,   // cheo -> thang 45
  TURN_D135_IN,   // thang -> cheo 135
  TURN_D135_OUT,  // cheo -> thang 135
  TURN_V90,       // cheo -> cheo 90 (hinh chu V)
  TURN_TYPE_COUNT
};

struct TurnSpec {
  float radius;      // mm, ban kinh doan cong deu (F180 duoc tinh lai tu dong)
  float transition;  // mm, do dai doan chuyen tiep hinh sin
};

struct TurnParams {
  float angle = 0;  // do, duong
  float radius = 0, transition = 0, arc = 0;
  float before = 0, after = 0;  // mm, co the am
  float minX = 0;               // (debug) toa do nho nhat theo truc x trong khung o vao
  bool valid = false;
  float length() const { return 2.0f * transition + arc; }
};

const char* turnName(TurnType t);

// Diem moc (khung toa do luc vao: goc tai diem moc vao, huong +x, re trai = +y)
void turnTarget(TurnType t, float halfCell, float& X, float& Y, float& angleDeg);

bool computeTurnParams(TurnType t, const TurnSpec& spec, float halfCell, TurnParams& out);

// Do cong chuan hoa [0..1] tai quang duong s (mm) tinh tu dau duong cong.
float curveShape(const TurnParams& p, float s);

}  // namespace mm
