// =============================================================================
//  Motion - vong dieu khien 500 Hz + cac lenh chuyen dong muc cao
//
//  Moi chu ky (ngat TIM2):
//    encoders -> cam bien -> cap nhat profile -> PD + feed-forward + bam tam -> PWM
//
//  Hai "kenh" doc lap: tien (forward, mm) va xoay (rotation, deg).
//  Sai so vi tri tich luy: err += (buoc dat - buoc do) moi chu ky, nen co the
//  noi tiep / thay profile bat ky luc nao ma khong giat.
// =============================================================================
#pragma once
#include <Arduino.h>

#include "profile.h"
#include "turns.h"

namespace motion {

void begin();             // khoi tao timer ngat dieu khien
void enable();            // reset sai so, bat dieu khien dong co
void disable();           // tha troi dong co
bool isEnabled();

// ----- Lenh (an toan khi goi tu main) -----
void resetProfiles();
void extendForward(float distance, float topSpeed, float finalSpeed, float accel);
void startTurn(float angle, float omega, float alpha);  // quay (tai cho neu dang dung)
void rebaseForward();
// Re slalom: robot giu toc do hien tai, goc quay bam theo quang duong (hinh sin - deu - hinh sin).
// Goi ngay sau khi doan thang truoc do ket thuc (forwardDone). Ket thuc: moc tien += length.
void startCurve(const mm::TurnParams& p, int8_t dir);
bool curveDone();
void setSteering(bool on);
void setManualSpeeds(float v, float omega);  // can chinh theo cam bien
void stopManual();

bool forwardDone();
bool turnDone();
float forwardPosition();
float forwardSpeed();

// Robot bi ket / va cham (sai so qua lon) -> tu dong tat dong co.
bool crashed();
void clearCrash();

}  // namespace motion
