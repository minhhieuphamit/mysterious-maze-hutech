// Phan tu dien dich duong di (translate / dictionary) port tu Bulebule mmlib path.c,
// Copyright (c) Bulebots, GPL-3.0. Phan con lai: GPL-3.0 theo project nay.
#include "smooth_path.h"

#include "maze_types.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace mm {

namespace {

constexpr float kSqrt2 = 1.41421356237f;

enum PathState : uint8_t { ORTHOGONAL = 0, DIAGONAL = 1 };

struct Translation {
  const char* from;  // mau can khop ('F' khop ca 'F' lan 'S')
  MoveKind kind;
  TurnType turn;
  int8_t dir;
};

#define T_(from, turn, dir) {from, MK_TURN, turn, dir}
#define DIAG_(from) {from, MK_DIAG, TURN_SS90, 0}
#define END_ {"", MK_END, TURN_SS90, 0}

const Translation kSafeOrtho[] = {
    T_("LF", TURN_F90, 1), T_("RF", TURN_F90, -1), T_("LLF", TURN_F180, 1), T_("RRF", TURN_F180, -1), END_};
const Translation kSafeDiag[] = {T_("LL", TURN_SS90, 1),  T_("RR", TURN_SS90, -1), T_("LR", TURN_SS90, 1),
                                 T_("RL", TURN_SS90, -1), T_("LF", TURN_SS90, 1),  T_("RF", TURN_SS90, -1),
                                 END_};
const Translation kDiagOrtho[] = {T_("LF", TURN_F90, 1),       T_("RF", TURN_F90, -1),
                                  T_("LR", TURN_D45_IN, 1),    T_("RL", TURN_D45_IN, -1),
                                  T_("LLF", TURN_F180, 1),     T_("RRF", TURN_F180, -1),
                                  T_("LLR", TURN_D135_IN, 1),  T_("RRL", TURN_D135_IN, -1),
                                  END_};
const Translation kDiagDiag[] = {DIAG_("LR"),
                                 DIAG_("RL"),
                                 T_("LF", TURN_D45_OUT, 1),
                                 T_("RF", TURN_D45_OUT, -1),
                                 T_("LLR", TURN_V90, 1),
                                 T_("RRL", TURN_V90, -1),
                                 T_("LLF", TURN_D135_OUT, 1),
                                 T_("RRF", TURN_D135_OUT, -1),
                                 END_};

const Translation* dictionary(PathLanguage lang, PathState st) {
  switch (lang) {
    case LANG_SAFE: return st == ORTHOGONAL ? kSafeOrtho : kSafeDiag;
    case LANG_DIAGONALS: return st == ORTHOGONAL ? kDiagOrtho : kDiagDiag;
    default: return nullptr;
  }
}

bool matches(const char* src, const char* pattern) {
  for (; *pattern; pattern++, src++) {
    if (*src == 0) return false;
    if (*pattern == 'F') {
      if (*src != 'F' && *src != 'S') return false;
    } else if (*src != *pattern) {
      return false;
    }
  }
  return true;
}

const Translation* translate(const char* src, PathLanguage lang, PathState st) {
  const Translation* t = dictionary(lang, st);
  if (!t) return nullptr;
  for (; t->from[0]; t++)
    if (matches(src, t->from)) return t;
  return nullptr;
}

bool turnExitsDiagonal(TurnType t) { return t == TURN_D45_IN || t == TURN_D135_IN || t == TURN_V90; }

// Thoi gian chay doan thang dai L, tu v0, toc do toi da vmax, ket thuc v1 (gia toc a).
float straightTime(float L, float v0, float vmax, float v1, float a) {
  if (L <= 0) return 0;
  float vp = sqrtf((2 * a * L + v0 * v0 + v1 * v1) * 0.5f);
  if (vp > vmax) vp = vmax;
  const float dAcc = (vp * vp - v0 * v0) / (2 * a);
  const float dDec = (vp * vp - v1 * v1) / (2 * a);
  const float cruise = L - dAcc - dDec;
  float t = (vp - v0) / a + (vp - v1) / a;
  if (cruise > 0 && vp > 0) t += cruise / vp;
  return t;
}

struct TimeCtx {
  float t;
  const RunConfig* cfg;
  float v;
};

bool timeSink(const Segment& s, void* ctx) {
  TimeCtx& c = *static_cast<TimeCtx*>(ctx);
  switch (s.kind) {
    case SEG_STRAIGHT: {
      float vend = s.vend;
      const float reach = sqrtf(c.v * c.v + 2 * c.cfg->accel * fmaxf(s.length, 0));
      if (vend > reach) vend = reach;
      c.t += straightTime(s.length, c.v, s.vmax, vend, c.cfg->accel);
      c.v = vend;
      break;
    }
    case SEG_CURVE:
      c.t += c.cfg->turn[s.turn].length() / fmaxf(s.speed, 1.0f);
      break;
    case SEG_INPLACE:
      c.t += 0.5f;
      c.v = 0;
      break;
  }
  return true;
}

}  // namespace

bool RunConfig::init(const TurnSpec specs[TURN_TYPE_COUNT]) {
  bool ok = true;
  for (uint8_t i = 0; i < TURN_TYPE_COUNT; i++) {
    ok &= computeTurnParams(TurnType(i), specs[i], halfCell, turn[i]);
    float v = sqrtf(lateralAccel * turn[i].radius);
    if (v > maxTurnSpeed) v = maxTurnSpeed;
    turnSpeed[i] = v;
  }
  // SS90 phai nam tron trong 1 o (before/after >= 0)
  ok &= turn[TURN_SS90].before >= 0 && turn[TURN_SS90].after >= 0;
  return ok;
}

uint16_t smoothPath(const char* src, PathLanguage lang, SmoothMove* out, uint16_t maxOut) {
  uint16_t n = 0;
  auto emit = [&](MoveKind k, TurnType t, int8_t d) {
    if (n + 1 < maxOut) out[n++] = SmoothMove{k, t, d};
  };
  PathState st = ORTHOGONAL;
  while (*src) {
    const char c = *src;
    switch (c) {
      case 'l': emit(MK_INPLACE, TURN_SS90, 1); src++; continue;
      case 'r': emit(MK_INPLACE, TURN_SS90, -1); src++; continue;
      case 'a': emit(MK_INPLACE, TURN_SS90, 2); src++; continue;
      case 'B': emit(MK_START, TURN_SS90, 0); st = ORTHOGONAL; src++; continue;
      case 'S': emit(MK_STOP, TURN_SS90, 0); src++; continue;
      case 'F': emit(MK_FRONT, TURN_SS90, 0); st = ORTHOGONAL; src++; continue;
      case 'L':
      case 'R': break;
      default: src++; continue;  // bo qua ky tu la
    }
    const Translation* t = (lang == LANG_SEARCH) ? nullptr : translate(src, lang, st);
    if (!t && lang != LANG_SEARCH && st == ORTHOGONAL) {
      st = DIAGONAL;  // nhu Bulebule: khong khop o trang thai thang -> thu tu dien "sau re"
      t = translate(src, lang, st);
    }
    if (!t) {
      // Khong khop (hoac LANG_SEARCH): re SS90 1 o
      emit(MK_TURN, TURN_SS90, c == 'L' ? 1 : -1);
      src++;
      st = DIAGONAL;
      continue;
    }
    emit(t->kind, t->turn, t->dir);
    src += strlen(t->from) - 1;  // ky tu cuoi cua mau chi de nhin truoc
    st = DIAGONAL;
  }
  out[n] = SmoothMove{MK_END, TURN_SS90, 0};
  return n;
}

bool walkSegments(const SmoothMove* m, const RunConfig& cfg, SegmentSink sink, void* ctx) {
  const float h = cfg.halfCell;
  float dist = 0;
  bool diag = false;
  bool atStartPose = true;
  float v = 0;  // toc do (de kiem tra quang phanh)
  auto straight = [&](float len, float vend) -> bool {
    if (len < -0.5f) return false;  // doan thang am: khong kha thi
    if (len < 0) len = 0;
    const float vmax = diag ? cfg.diagSpeed : cfg.topSpeed;
    if (vend > vmax) vend = vmax;
    // Phanh: tu toc do hien tai xuong vend trong len
    if (v > vend && (v * v - vend * vend) > 2 * cfg.accel * len + 1.0f) return false;
    Segment s{SEG_STRAIGHT, diag, len, vmax, vend, TURN_SS90, 0, 0};
    if (sink && !sink(s, ctx)) return false;
    const float reach = sqrtf(v * v + 2 * cfg.accel * len);
    v = vend < reach ? vend : reach;
    return true;
  };
  for (; m->kind != MK_END; m++) {
    switch (m->kind) {
      case MK_INPLACE: {
        if (atStartPose) {
          if (!straight(cfg.startToCenter, 0)) return false;
          atStartPose = false;
        }
        Segment s{SEG_INPLACE, false, 0, 0, 0, TURN_SS90, m->dir, 0};
        if (sink && !sink(s, ctx)) return false;
        v = 0;
        break;
      }
      case MK_START:
        dist = atStartPose ? cfg.startToEdge() : h;
        atStartPose = false;
        diag = false;
        break;
      case MK_FRONT:
        dist += 2 * h;
        diag = false;
        break;
      case MK_DIAG:
        dist += kSqrt2 * h;
        diag = true;
        break;
      case MK_TURN: {
        const TurnParams& tp = cfg.turn[m->turn];
        const float vt = cfg.turnSpeed[m->turn];
        if (!straight(dist + tp.before, vt)) return false;
        Segment s{SEG_CURVE, diag, tp.length(), 0, 0, m->turn, m->dir, vt};
        if (sink && !sink(s, ctx)) return false;
        dist = tp.after;
        diag = turnExitsDiagonal(m->turn);
        break;
      }
      case MK_STOP:
        if (!straight(dist + h, 0)) return false;
        dist = 0;
        break;
      default: break;
    }
  }
  return true;
}

float estimateRunTime(const SmoothMove* moves, const RunConfig& cfg) {
  TimeCtx c{0, &cfg, 0};
  if (!walkSegments(moves, cfg, timeSink, &c)) return -1;
  return c.t;
}

uint16_t buildSmoothPath(const char* moves, const RunConfig& cfg, PathLanguage maxLang, SmoothMove* out,
                         uint16_t maxOut, PathLanguage* used, float* estTime) {
  static SmoothMove tmp[kMaxSmoothMoves];
  uint16_t bestN = 0;
  float bestT = -1;
  PathLanguage bestLang = LANG_SEARCH;
  for (int l = maxLang; l >= 0; l--) {
    uint16_t n = smoothPath(moves, PathLanguage(l), tmp, sizeof(tmp) / sizeof(tmp[0]));
    float t = estimateRunTime(tmp, cfg);
    if (t < 0) continue;
    if (bestT < 0 || t < bestT) {
      bestT = t;
      bestLang = PathLanguage(l);
      bestN = n < maxOut - 1 ? n : uint16_t(maxOut - 1);
      memcpy(out, tmp, bestN * sizeof(SmoothMove));
      out[bestN] = SmoothMove{MK_END, TURN_SS90, 0};
    }
  }
  if (used) *used = bestLang;
  if (estTime) *estTime = bestT;
  return bestN;
}

const char* moveName(const SmoothMove& m, char* buf, uint8_t len) {
  switch (m.kind) {
    case MK_START: snprintf(buf, len, "START"); break;
    case MK_STOP: snprintf(buf, len, "STOP"); break;
    case MK_FRONT: snprintf(buf, len, "F"); break;
    case MK_DIAG: snprintf(buf, len, "D"); break;
    case MK_INPLACE: snprintf(buf, len, "SPIN%c", m.dir == 2 ? 'B' : (m.dir > 0 ? 'L' : 'R')); break;
    case MK_TURN: snprintf(buf, len, "%s%c", turnName(m.turn), m.dir > 0 ? 'L' : 'R'); break;
    default: snprintf(buf, len, "END"); break;
  }
  return buf;
}

}  // namespace mm
