// =============================================================================
//  Planner - tim duong cho tham do va cho luot chay toc do
//
//  1) planPath(): Dijkstra tren (o, huong), cost = di thang / re. Nhanh (1024 trang
//     thai, heap) -> dung trong luc tham do (robot dang chay).
//  2) buildFastRun(): Dijkstra tren (o, huong, nuoc di truoc) voi cost = THOI GIAN
//     (ms) tinh tu RunConfig: hieu duoc zig-zag re (chay cheo), re lien tiep, doan
//     thang dai. Sau do thu ca duong cua (1) va chon duong co uoc luong nhanh nhat.
// =============================================================================
#pragma once
#include "maze.h"
#include "smooth_path.h"

namespace mm {

struct PlannerCosts {
  uint16_t straight = 10;
  uint16_t turn = 20;  // re tai cho: ~20-30 ; re cong (smooth turn): ~8-12
};

// Tim duong tu start (dang huong startHeading) toi BAT KY o nao trong targets.
// Ghi chuoi o (bao gom ca o dau va o dich) vao path. Tra ve so o; 0 = khong co duong.
uint16_t planPath(const Maze& maze, Location start, Heading startHeading, const Location* targets,
                  uint8_t targetCount, Maze::Mask mask, const PlannerCosts& costs, Location* path,
                  uint16_t maxLen, uint32_t* totalCost = nullptr);

// Chuyen chuoi o thanh chuoi lenh (he quy chieu "canh o"):
//   'l' 'r' 'a' : quay tai cho o o xuat phat (trai / phai / 180) - hiem khi dung
//   'B'         : tu vi tri xuat phat toi canh o dau tien
//   'F'         : di thang qua 1 o (canh -> canh)
//   'L' 'R'     : re trai / phai trong o (canh -> canh)
//   'S'         : vao o dich va dung o tam o
uint16_t buildMoves(const Location* path, uint16_t count, Heading startHeading, char* out, uint16_t maxLen);

// Duong ngan nhat theo PlannerCosts tren ban do DA BIET (mask CLOSED) -> chuoi lenh.
uint16_t buildSpeedRun(const Maze& maze, Heading startHeading, const PlannerCosts& costs, char* out,
                       uint16_t maxLen, uint32_t* totalCost = nullptr);

// Duong NHANH NHAT theo thoi gian uoc luong cho RunConfig + ngon ngu toi da maxLang.
// out nhan chuoi lenh (B..S), smooth nhan chuoi chuyen dong da dich. Tra ve so phan tu
// cua smooth (0 = khong co duong). estTime = giay.
uint16_t buildFastRun(const Maze& maze, const RunConfig& cfg, PathLanguage maxLang, char* out, uint16_t maxLen,
                      SmoothMove* smooth, uint16_t maxSmooth, float* estTime = nullptr,
                      PathLanguage* used = nullptr);

// Ban do da "giai xong" chua: duong tot nhat chi qua tuong da biet (CLOSED) co cost
// bang duong lac quan (OPEN)? Neu bang -> khong can tham do them.
// optimistic = OPEN (mac dinh) hoac HINT (tin ban do goi y o nhung cho chua do).
bool isMazeSolved(const Maze& maze, Heading startHeading, const PlannerCosts& costs,
                  Maze::Mask optimistic = Maze::OPEN);

}  // namespace mm
