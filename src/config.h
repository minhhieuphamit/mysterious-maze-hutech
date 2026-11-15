// =============================================================================
//  CAU HINH ROBOT - chinh sua file nay cho dung phan cung cua doi.
//  Don vi: mm, mm/s, mm/s^2, do (deg), deg/s, deg/s^2, Volt.
// =============================================================================
#pragma once
#include <Arduino.h>

#include "run_defaults.h"

// -----------------------------------------------------------------------------
//  1. KICH THUOC ME CUNG (theo the le MYSTERIOUS MAZE 2026)
//  !!! Khac chuan quoc te: o 18x18 cm la KHOANG LOT LONG, tuong day 1.2 cm
//      => buoc o (tam o -> tam o) = 180 + 12 = 192 mm.
// -----------------------------------------------------------------------------
constexpr float CELL_INTERIOR = 180.0f;
constexpr float WALL_THICKNESS = 12.0f;
constexpr float CELL_PITCH = CELL_INTERIOR + WALL_THICKNESS;  // 192
constexpr float HALF_CELL = CELL_PITCH / 2.0f;                // 96

// Khoang cach tu MAT SAU robot (cham tuong) toi tam truc banh xe. Do tren robot that.
constexpr float BACK_WALL_TO_CENTER = 40.0f;
// Tu vi tri xuat phat (lung cham tuong) -> tam o (0,0) va -> canh o (0,0)/(0,1)
constexpr float START_TO_CENTER = CELL_INTERIOR / 2.0f - BACK_WALL_TO_CENTER;
constexpr float START_TO_EDGE = START_TO_CENTER + HALF_CELL;

// Diem doc cam bien khi tham do: X mm sau khi qua canh o.
// Tai day cam bien ben phai nhin thay tuong cua o moi. Chinh theo vi tri cam bien.
// Phai NHO HON before cua SS90 (~18 mm) neu dung re cong khi tham do.
constexpr float SENSING_OFFSET = 8.0f;

// O xuat phat: luat quoc te co tuong 3 mat (chi mo phia Bac). true = gia dinh san.
constexpr bool START_CELL_HAS_EAST_WALL = true;

// -----------------------------------------------------------------------------
//  2. CO KHI / DONG CO (mac dinh: N20 6V 1:20 ~1500 rpm + encoder tu 7 xung/vong, banh 34 mm)
//  Toc do toi da khong tai ~ rpm/60 * PI * D: 1:20 -> ~2.7 m/s ; 1:30 -> ~1.8 m/s (chi du cho ~1.3 m/s).
// -----------------------------------------------------------------------------
constexpr float WHEEL_DIAMETER = 34.0f;
constexpr float ENCODER_PULSES = 7.0f;     // xung / vong truc dong co (1 kenh)
constexpr float GEAR_RATIO = 20.0f;      // doi theo dong co that (10 / 20 / 30 ...)
constexpr float WHEEL_SEPARATION = 74.0f;  // khoang cach 2 tam banh (calib bang mode SPIN)
// He so hieu chinh tung banh (calib bang mode STRAIGHT: chay 4 o, do sai so)
constexpr float LEFT_WHEEL_ADJUST = 1.000f;
constexpr float RIGHT_WHEEL_ADJUST = 1.000f;

constexpr float COUNTS_PER_REV = ENCODER_PULSES * 4.0f * GEAR_RATIO;  // x4: dem ca 2 suon 2 kenh
constexpr float MM_PER_COUNT = PI * WHEEL_DIAMETER / COUNTS_PER_REV;
constexpr float MM_PER_COUNT_LEFT = MM_PER_COUNT * LEFT_WHEEL_ADJUST;
constexpr float MM_PER_COUNT_RIGHT = MM_PER_COUNT * RIGHT_WHEEL_ADJUST;
constexpr float DEG_PER_MM_DIFFERENCE = 180.0f / (PI * WHEEL_SEPARATION);

// Dao chieu neu banh/encoder quay nguoc (kiem tra bang mode SENSOR/ENCODER)
constexpr bool LEFT_MOTOR_REVERSED = false;
constexpr bool RIGHT_MOTOR_REVERSED = true;
constexpr bool LEFT_ENCODER_REVERSED = false;
constexpr bool RIGHT_ENCODER_REVERSED = true;

// -----------------------------------------------------------------------------
//  3. VONG DIEU KHIEN
// -----------------------------------------------------------------------------
#if defined(STM32F4xx)
constexpr uint32_t LOOP_FREQUENCY = 1000;  // Hz (F411 du nhanh)
#else
constexpr uint32_t LOOP_FREQUENCY = 500;  // Hz (F103)
#endif
constexpr float LOOP_INTERVAL = 1.0f / LOOP_FREQUENCY;

// PD vi tri cho chuyen dong tien (V / mm) va xoay (V / deg)
constexpr float FWD_KP = 0.10f;
constexpr float FWD_KD = 1.00f;  // (nhan voi sai so thay doi moi chu ky)
constexpr float ROT_KP = 0.07f;
constexpr float ROT_KD = 0.80f;

// Feed-forward dong co (do bang cach cho chay toc do co dinh, xem docs/04-calib-va-tinh-chinh.md)
constexpr float SPEED_FF = 0.0025f;  // V / (mm/s) cua banh xe (~ 6 V / toc do khong tai 2.7 m/s cua N20 1:20)
constexpr float ACC_FF = 0.0005f;    // V / (mm/s^2)
constexpr float BIAS_FF = 0.20f;     // V thang ma sat tinh

constexpr float MAX_MOTOR_VOLTS = 6.0f;  // dong co N20 6V
constexpr uint32_t MOTOR_PWM_HZ = 20000;
constexpr int MOTOR_PWM_MAX = 1023;  // 10 bit

// Phat hien va cham / ket: sai so vuot nguong -> dung khan cap
constexpr float FWD_ERROR_LIMIT = 40.0f;  // mm
constexpr float ROT_ERROR_LIMIT = 30.0f;  // deg

// -----------------------------------------------------------------------------
//  4. CAM BIEN HONG NGOAI (4 kenh: trai ben, trai truoc, phai truoc, phai ben)
//  Mach: phototransistor C -> 3.3V, E -> ADC + dien tro xuong GND (cang sang -> ADC cang cao).
//  Gia tri sau chuan hoa: dat robot GIUA o -> ben trai/phai ~ 100.
//  Dung mode SENSOR doc gia tri tho roi dien SCALE = 100 / raw.
// -----------------------------------------------------------------------------
constexpr float LSS_SCALE = 0.100f;
constexpr float LFS_SCALE = 0.100f;
constexpr float RFS_SCALE = 0.100f;
constexpr float RSS_SCALE = 0.100f;

constexpr float SIDE_NOMINAL = 100.0f;      // gia tri ben khi o giua o
constexpr float FRONT_REFERENCE = 200.0f;   // lfs + rfs khi dung TAM o, mat tuong truoc (moi ben ~100 sau calib)
constexpr float LEFT_WALL_THRESHOLD = 40.0f;
constexpr float RIGHT_WALL_THRESHOLD = 40.0f;
constexpr float FRONT_WALL_THRESHOLD = 20.0f;  // (lfs + rfs)/2 tai diem doc cam bien
constexpr uint32_t EMITTER_SETTLE_US = 40;

// Bam tam duong (steering) dua tren sai lech 2 cam bien ben
constexpr float STEERING_KP = 1.2f;    // deg/s cho moi don vi sai lech
constexpr float STEERING_KD = 4.0f;
constexpr float STEERING_MAX = 120.0f; // deg/s

// Can chinh voi tuong truoc khi dung (giam sai so tich luy)
constexpr float ALIGN_FWD_KP = 4.0f;   // mm/s moi don vi
constexpr float ALIGN_ROT_KP = 3.0f;   // deg/s moi don vi
constexpr uint32_t ALIGN_TIME_MS = 250;

// Bat dau bang cach "vay tay" truoc cam bien truoc
constexpr float START_HAND_THRESHOLD = 150.0f;

// Pin: phan ap do dien ap pin. LiPo 2S day = 8.4 V -> can chia >= 2.6 de ADC <= 3.3 V.
// Mac dinh 20k (tren) / 10k (duoi) -> chia 3.0 (8.4 V -> 2.8 V).
constexpr float BATTERY_DIVIDER = 3.0f;
constexpr float ADC_REF_VOLTS = 3.3f;
constexpr float BATTERY_LOW_VOLTS = 6.8f;  // LiPo 2S

// -----------------------------------------------------------------------------
//  5. THONG SO CHAY
// -----------------------------------------------------------------------------
// ---- Tham do (search) ----
struct SearchParams {
  float speed;      // mm/s toc do chay deu khi tham do
  float accel;      // mm/s^2
  float turnSpeed;  // deg/s quay tai cho (quay dau / khi khong dung re cong)
  float turnAccel;  // deg/s^2
  bool smoothTurns; // re cong SS90 khong dung lai (nhanh hon nhieu); false = dung + quay tai cho
};
// Vong loai chi can toi dich trong 3 phut -> on dinh truoc, nhanh sau.
constexpr SearchParams SEARCH_PARAMS = {350.0f, 1500.0f, 360.0f, 2400.0f, true};

// ---- Luot chay toc do (speed run) ----
//  maxLang: 0 = chi re SS90, 1 = them F90/F180, 2 = them chay cheo (45/135/V90)
//  lateralAccel: gia toc huong tam khi re -> toc do re = sqrt(lateralAccel * R).
//  Tang dan tung muc khi robot da chay on dinh. Nen truot banh -> giam lateralAccel.
struct RunParams {
  float topSpeed;      // mm/s doan thang truc giao
  float diagSpeed;     // mm/s doan cheo
  float accel;         // mm/s^2
  float lateralAccel;  // mm/s^2
  float maxTurnSpeed;  // mm/s
  uint8_t maxLang;
};
constexpr RunParams RUN_PARAMS[3] = {
    {1000.0f, 700.0f, 2500.0f, 3500.0f, 700.0f, 0},     // RUN 1: an toan, chi SS90
    {1600.0f, 1000.0f, 4000.0f, 6000.0f, 1000.0f, 1},   // RUN 2: F90 / F180
    {2200.0f, 1500.0f, 5500.0f, 9000.0f, 1500.0f, 2},   // RUN 3: day du + cheo
};
constexpr uint8_t RUN_PARAM_COUNT = 3;

// Ban kinh re (mm). before/after tu tinh (MazeCore/turns.cpp). Gia tri mac dinh: run_defaults.h.
// Muon doi: sua bang nay, chay test_speedrun.exe de kiem tra khe ho tuong.
#define TURN_SPECS mm::kDefaultTurnSpecs

// Trong so planner luc tham do (khong anh huong speed run)
constexpr uint16_t PLAN_COST_STRAIGHT = 10;
constexpr uint16_t PLAN_COST_TURN = 15;

// Sau khi toi dich: di them toi da bao nhieu o de tim duong toi uu (moi o ~0.6 s khi tham do).
// Me cung mau cua BTC can ~170 o them de chac chan tim ra duong toi uu.
constexpr uint16_t EXPLORE_MAX_CELLS = 300;
// Tu dung tham do sau X ms ke tu luc xuat phat (luot gioi han 3 phut, tinh ca chuan bi).
// Ban do da biet van duoc luu -> luot sau chay toc do.
constexpr uint32_t SEARCH_TIME_LIMIT_MS = 150000;
// Che do SEARCH+MAP: so tuong do duoc khac ban do nap san toi da truoc khi bo ban do.
constexpr uint16_t PRESET_MISMATCH_TOLERANCE = 4;

// -----------------------------------------------------------------------------
//  6. SO DO CHAN (STM32F411 BlackPill / STM32F103 BluePill - chung chan)
// -----------------------------------------------------------------------------
// Encoder: dung bo dem phan cung (TIMx encoder mode)
//   Trai : TIM4  PB6 / PB7      Phai : TIM3  PA6 / PA7
// Driver TB6612FNG
constexpr uint32_t PIN_PWM_LEFT = PA8;    // TIM1_CH1
constexpr uint32_t PIN_PWM_RIGHT = PA9;   // TIM1_CH2
constexpr uint32_t PIN_LEFT_IN1 = PB14;
constexpr uint32_t PIN_LEFT_IN2 = PB15;
constexpr uint32_t PIN_RIGHT_IN1 = PB5;
constexpr uint32_t PIN_RIGHT_IN2 = PB10;
// (STBY cua TB6612 noi thang len 3.3V)

// Cam bien IR (ADC) va LED phat (qua MOSFET)
constexpr uint32_t PIN_SENSOR_LSS = PA0;  // trai ben  (BlackPill: dung nhan nut KEY khi chay)
constexpr uint32_t PIN_SENSOR_LFS = PA1;  // trai truoc
constexpr uint32_t PIN_SENSOR_RFS = PA4;  // phai truoc
constexpr uint32_t PIN_SENSOR_RSS = PA5;  // phai ben
constexpr uint32_t PIN_EMITTER_SIDE = PB12;
constexpr uint32_t PIN_EMITTER_FRONT = PB13;
constexpr uint32_t PIN_BATTERY = PB0;

// Giao dien
constexpr uint32_t PIN_BUTTON_GO = PB8;    // keo len, nhan = LOW
constexpr uint32_t PIN_BUTTON_MODE = PB9;
constexpr uint32_t PIN_LED = PC13;         // LED tren board, sang khi LOW
constexpr uint32_t PIN_BUZZER = PB1;       // coi tich cuc (active buzzer), HIGH = keu
// Debug UART qua day (USB-UART). KHONG dung khong day khi thi dau.
constexpr uint32_t PIN_DEBUG_TX = PA2;
constexpr uint32_t PIN_DEBUG_RX = PA3;
constexpr uint32_t DEBUG_BAUD = 115200;
