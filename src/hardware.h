// =============================================================================
//  Lop phan cung muc thap: encoder, dong co, cam bien IR, nut/LED/coi.
// =============================================================================
#pragma once
#include <Arduino.h>

// ------------------------------------------------------------------ Encoders
// Dem bang TIM3/TIM4 o che do encoder (phan cung, khong ton CPU, khong mat xung).
namespace encoders {
void begin();
void update();  // goi trong ngat dieu khien
void reset();
extern volatile float fwdChange;   // mm trong chu ky vua roi
extern volatile float rotChange;   // deg trong chu ky vua roi (+ = quay trai / CCW)
extern volatile float distance;    // mm tich luy
extern volatile float angle;       // deg tich luy
extern volatile int32_t leftCount, rightCount;
}  // namespace encoders

// ------------------------------------------------------------------ Motors
namespace motors {
void begin();
void setVolts(float left, float right, float battery);
void coast();
}  // namespace motors

// ------------------------------------------------------------------ IR sensors
namespace sensors {
void begin();
void update();  // goi trong ngat dieu khien: doc toi -> bat LED -> doc sang
void setEnabled(bool on);
extern volatile float lss, lfs, rfs, rss;              // gia tri da chuan hoa
extern volatile int rawLss, rawLfs, rawRfs, rawRss;    // gia tri tho (lit - dark)
extern volatile float batteryVolts;
bool leftWall();
bool rightWall();
bool frontWall();
float frontSum();
float crossTrackError();  // >0: robot lech ve phia tuong TRAI
}  // namespace sensors

// ------------------------------------------------------------------ UI
namespace ui {
void begin();
bool goPressed();
bool modePressed();
bool waitRelease(uint32_t pin);
void led(bool on);
void beep(uint16_t ms);
void beeps(uint8_t n);
void blink(uint8_t n);
}  // namespace ui

#if defined(STM32_CORE_VERSION_MAJOR) && (STM32_CORE_VERSION_MAJOR >= 3)
extern Uart DebugSerial;
#else
extern HardwareSerial DebugSerial;
#endif
