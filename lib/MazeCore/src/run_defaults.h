// Thong so re mac dinh cho o buoc 192 mm (BTC Mysterious Maze: 180 lot long + 12 tuong).
// Diem xuat phat: ban kinh / transition cua Bulebule (o 180 mm) nhan 192/180.
// before/after duoc tinh tu dong -> chi can chinh ban kinh.
#pragma once
#include "turns.h"

namespace mm {

constexpr TurnSpec kDefaultTurnSpecs[TURN_TYPE_COUNT] = {
    {52.5f, 64.4f},   // SS90  (tham do)
    {138.7f, 64.4f},  // F90
    {94.7f, 64.4f},   // F180  (ban kinh tu tinh lai)
    {106.7f, 64.4f},  // D45 vao
    {106.7f, 64.4f},  // D45 ra
    {85.3f, 64.4f},   // D135 vao
    {85.3f, 64.4f},   // D135 ra
    {69.3f, 64.4f},   // V90
};

}  // namespace mm
