// =============================================================================
//  SmoothPath - chuyen chuoi o (B F L R S) thanh chuoi chuyen dong toc do cao:
//  doan thang, doan cheo va cac kieu re slalom (F90, F180, 45/135 vao/ra cheo, V90).
//
//  Bo tu dien dich duong di port tu Bulebule/mmlib path.c
//  (Copyright Bulebots, GPL-3.0, https://github.com/Bulebots/mmlib), mo rong:
//    - 'S' (dung o dich) duoc coi nhu 'F' khi nhin truoc
//    - kiem tra kha thi (doan thang khong am, du quang phanh) va tu dong ha
//      xuong ngon ngu an toan hon neu khong kha thi
//    - uoc luong thoi gian chay
// =============================================================================
#pragma once
#include "maze_types.h"
#include "turns.h"

namespace mm {

// So phan tu toi da cua 1 chuoi chuyen dong (moi o sinh toi da 1 phan tu)
constexpr uint16_t kMaxSmoothMoves = kCellCount + 16;

enum MoveKind : uint8_t { MK_END = 0, MK_START, MK_STOP, MK_FRONT, MK_DIAG, MK_TURN, MK_INPLACE };

struct SmoothMove {
  MoveKind kind;
  TurnType turn;
  int8_t dir;  // +1 trai, -1 phai (MK_INPLACE: +1 trai, -1 phai, 2 = 180)
};

enum PathLanguage : uint8_t {
  LANG_SEARCH = 0,  // moi lan re = SS90 (cham, chac chan)
  LANG_SAFE,        // F90 / F180 khi co doan thang, SS90 cho zig-zag (khong cheo)
  LANG_DIAGONALS,   // day du: cheo, 45/135, V90
  LANG_COUNT
};

// Thong so 1 kieu chay (dung chung cho planner, kiem tra, uoc luong, firmware).
struct RunConfig {
  float halfCell = 96.0f;
  float startToCenter = 50.0f;  // vi tri xuat phat -> tam o (0,0)
  float topSpeed = 1500.0f;     // mm/s, doan thang truc giao
  float diagSpeed = 1000.0f;    // mm/s, doan cheo
  float accel = 4000.0f;        // mm/s^2 (tang & giam toc)
  float lateralAccel = 8000.0f; // mm/s^2: gia toc huong tam toi da khi re (do bam banh)
  float maxTurnSpeed = 1500.0f;
  TurnParams turn[TURN_TYPE_COUNT];
  float turnSpeed[TURN_TYPE_COUNT] = {};

  float startToEdge() const { return startToCenter + halfCell; }
  // Tinh hinh hoc + toc do re tu ban kinh. Tra ve false neu co kieu re loi.
  bool init(const TurnSpec specs[TURN_TYPE_COUNT]);
};

// Doan chuyen dong co ban sinh ra tu chuoi SmoothMove.
enum SegKind : uint8_t { SEG_STRAIGHT, SEG_CURVE, SEG_INPLACE };
struct Segment {
  SegKind kind;
  bool diagonal;   // SEG_STRAIGHT: dang chay cheo (tat bam tuong ben)
  float length;    // SEG_STRAIGHT: mm
  float vmax;      // SEG_STRAIGHT: toc do toi da
  float vend;      // SEG_STRAIGHT: toc do cuoi (toc do vao re, hoac 0)
  TurnType turn;   // SEG_CURVE
  int8_t dir;      // SEG_CURVE / SEG_INPLACE
  float speed;     // SEG_CURVE: toc do re
};

typedef bool (*SegmentSink)(const Segment& seg, void* ctx);

// Dich chuoi o -> chuyen dong. Tra ve so phan tu (khong tinh MK_END).
uint16_t smoothPath(const char* moves, PathLanguage lang, SmoothMove* out, uint16_t maxOut);

// Duyet chuoi chuyen dong thanh cac Segment. Dung sink = nullptr de chi kiem tra.
// Tra ve false neu khong kha thi (doan thang am / khong du quang phanh) hoac sink tra false.
bool walkSegments(const SmoothMove* moves, const RunConfig& cfg, SegmentSink sink, void* ctx);

// Uoc luong thoi gian chay (giay), ly tuong (khong truot). < 0 neu khong kha thi.
float estimateRunTime(const SmoothMove* moves, const RunConfig& cfg);

// Thu lan luot DIAGONALS -> SAFE -> SEARCH (tu maxLang tro xuong), chon ngon ngu kha thi
// va NHANH NHAT theo uoc luong. Tra ve so phan tu, 0 neu that bai.
uint16_t buildSmoothPath(const char* moves, const RunConfig& cfg, PathLanguage maxLang, SmoothMove* out,
                         uint16_t maxOut, PathLanguage* used = nullptr, float* estTime = nullptr);

const char* moveName(const SmoothMove& m, char* buf, uint8_t bufLen);

}  // namespace mm
