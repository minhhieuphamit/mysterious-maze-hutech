#include "turns.h"

#include <math.h>

namespace mm {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kSqrt2 = 1.41421356237f;

// Tich phan quy dao cua duong cong (bat dau tai goc, huong +x).
void integrate(const TurnParams& p, float& a, float& b, float& minX) {
  const int kSteps = 2000;
  const float L = p.length();
  const float ds = L / kSteps;
  float phi = 0, x = 0, y = 0;
  minX = 0;
  for (int i = 0; i < kSteps; i++) {
    const float s = (i + 0.5f) * ds;
    const float k = curveShape(p, s) / p.radius;  // rad/mm
    const float mid = phi + 0.5f * k * ds;
    x += cosf(mid) * ds;
    y += sinf(mid) * ds;
    phi += k * ds;
    if (x < minX) minX = x;
  }
  a = x;
  b = y;
}

// Dat ban kinh + transition, tinh do dai doan arc de tong goc = angle.
void shape(TurnParams& p, float radius, float transition) {
  const float theta = p.angle * kPi / 180.0f;
  p.radius = radius;
  p.transition = transition;
  p.arc = theta * radius - 4.0f * transition / kPi;
  if (p.arc < 0) {  // transition qua dai so voi goc -> rut ngan
    p.transition = theta * radius * kPi / 4.0f;
    p.arc = 0;
  }
}
}  // namespace

const char* turnName(TurnType t) {
  static const char* const names[TURN_TYPE_COUNT] = {"SS90",   "F90",     "F180",     "D45in",
                                                     "D45out", "D135in", "D135out", "V90"};
  return t < TURN_TYPE_COUNT ? names[t] : "?";
}

void turnTarget(TurnType t, float h, float& X, float& Y, float& angle) {
  switch (t) {
    case TURN_SS90:
    case TURN_F90: X = h; Y = h; angle = 90; break;
    case TURN_F180: X = 0; Y = 2 * h; angle = 180; break;
    case TURN_D45_IN: X = h; Y = h; angle = 45; break;
    case TURN_D45_OUT: X = kSqrt2 * h; Y = 0; angle = 45; break;
    case TURN_D135_IN: X = 0; Y = 2 * h; angle = 135; break;
    case TURN_D135_OUT: X = kSqrt2 * h; Y = kSqrt2 * h; angle = 135; break;
    case TURN_V90: X = kSqrt2 * h; Y = kSqrt2 * h; angle = 90; break;
    default: X = Y = angle = 0; break;
  }
}

bool computeTurnParams(TurnType t, const TurnSpec& spec, float halfCell, TurnParams& out) {
  float X, Y, angle;
  turnTarget(t, halfCell, X, Y, angle);
  out = TurnParams();
  out.angle = angle;
  if (angle <= 0 || spec.radius <= 0) return false;

  float a = 0, b = 0, minX = 0;
  if (t == TURN_F180) {
    // Do lech ngang b phai bang Y -> tim ban kinh bang chia doi.
    float lo = 5.0f, hi = 4.0f * halfCell;
    for (int i = 0; i < 40; i++) {
      const float r = 0.5f * (lo + hi);
      shape(out, r, spec.transition);
      integrate(out, a, b, minX);
      if (b < Y)
        lo = r;
      else
        hi = r;
    }
    shape(out, 0.5f * (lo + hi), spec.transition);
    integrate(out, a, b, minX);
    // X = before + a - after. Chia deu: before - after = X - a (duong cong doi xung -> a ~ 0)
    out.before = 0.5f * (X - a);
    out.after = -0.5f * (X - a);
  } else {
    shape(out, spec.radius, spec.transition);
    integrate(out, a, b, minX);
    const float th = angle * kPi / 180.0f;
    out.after = (Y - b) / sinf(th);
    out.before = X - a - out.after * cosf(th);
  }
  out.minX = minX;
  out.valid = isfinite(out.before) && isfinite(out.after);
  return out.valid;
}

float curveShape(const TurnParams& p, float s) {
  const float tr = p.transition;
  if (s <= 0) return 0;
  if (tr <= 0) return (s <= p.arc) ? 1.0f : 0.0f;
  if (s < tr) return sinf(0.5f * kPi * s / tr);
  if (s < tr + p.arc) return 1.0f;
  if (s < 2 * tr + p.arc) return sinf(0.5f * kPi * (s - p.arc) / tr);
  return 0;
}

}  // namespace mm
