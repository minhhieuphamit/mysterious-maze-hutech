#include "hardware.h"

#include "config.h"

// STM32duino >= 3.0 doi lop UART cu the thanh `Uart`
#if defined(STM32_CORE_VERSION_MAJOR) && (STM32_CORE_VERSION_MAJOR >= 3)
Uart DebugSerial(PIN_DEBUG_RX, PIN_DEBUG_TX);
#else
HardwareSerial DebugSerial(PIN_DEBUG_RX, PIN_DEBUG_TX);
#endif

// =============================================================================
//  ENCODERS - TIM3 (phai: PA6/PA7), TIM4 (trai: PB6/PB7), encoder mode 3 (x4)
// =============================================================================
namespace encoders {

volatile float fwdChange = 0, rotChange = 0, distance = 0, angle = 0;
volatile int32_t leftCount = 0, rightCount = 0;
static uint16_t lastLeft = 0, lastRight = 0;

static void configureTimer(TIM_TypeDef* tim) {
  tim->CR1 = 0;
  tim->SMCR = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;  // encoder mode 3: dem ca TI1 va TI2
  // CC1/CC2 la input map vao TI1/TI2, loc nhieu IC1F/IC2F = 0b0011 (8 mau)
  tim->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0 | (0x3u << TIM_CCMR1_IC1F_Pos) | (0x3u << TIM_CCMR1_IC2F_Pos);
  tim->CCER = 0;
  tim->ARR = 0xFFFF;
  tim->CNT = 0;
  tim->CR1 = TIM_CR1_CEN;
}

void begin() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  GPIO_InitTypeDef g = {};
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
#if defined(STM32F1xx)
  g.Mode = GPIO_MODE_INPUT;  // F1: kenh timer doc truc tiep tu chan input
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOA, &g);
  HAL_GPIO_Init(GPIOB, &g);
#else
  g.Mode = GPIO_MODE_AF_PP;  // F4: chan phai o che do Alternate Function
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  g.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOA, &g);
  g.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOB, &g);
#endif
  configureTimer(TIM3);
  configureTimer(TIM4);
  reset();
}

void reset() {
  noInterrupts();
  lastLeft = TIM4->CNT;
  lastRight = TIM3->CNT;
  fwdChange = rotChange = distance = angle = 0;
  leftCount = rightCount = 0;
  interrupts();
}

void update() {
  const uint16_t l = TIM4->CNT;
  const uint16_t r = TIM3->CNT;
  int16_t dl = int16_t(l - lastLeft);
  int16_t dr = int16_t(r - lastRight);
  lastLeft = l;
  lastRight = r;
  if (LEFT_ENCODER_REVERSED) dl = -dl;
  if (RIGHT_ENCODER_REVERSED) dr = -dr;
  leftCount += dl;
  rightCount += dr;
  const float mmL = dl * MM_PER_COUNT_LEFT;
  const float mmR = dr * MM_PER_COUNT_RIGHT;
  fwdChange = 0.5f * (mmL + mmR);
  rotChange = (mmR - mmL) * DEG_PER_MM_DIFFERENCE;
  distance += fwdChange;
  angle += rotChange;
}

}  // namespace encoders

// =============================================================================
//  MOTORS - TB6612FNG, PWM 20 kHz tren TIM1 (PA8 = CH1 trai, PA9 = CH2 phai)
// =============================================================================
namespace motors {

static HardwareTimer* pwmTimer = nullptr;

void begin() {
  pinMode(PIN_LEFT_IN1, OUTPUT);
  pinMode(PIN_LEFT_IN2, OUTPUT);
  pinMode(PIN_RIGHT_IN1, OUTPUT);
  pinMode(PIN_RIGHT_IN2, OUTPUT);
  pwmTimer = new HardwareTimer(TIM1);
  pwmTimer->setMode(1, TIMER_OUTPUT_COMPARE_PWM1, PIN_PWM_LEFT);
  pwmTimer->setMode(2, TIMER_OUTPUT_COMPARE_PWM1, PIN_PWM_RIGHT);
  pwmTimer->setOverflow(MOTOR_PWM_HZ, HERTZ_FORMAT);
  pwmTimer->setCaptureCompare(1, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwmTimer->setCaptureCompare(2, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwmTimer->resume();
  coast();
}

static void drive(uint32_t channel, uint32_t in1, uint32_t in2, float volts, float battery, bool reversed) {
  if (reversed) volts = -volts;
  if (battery < 3.0f) battery = 3.0f;  // tranh chia cho 0 khi chua doc duoc pin
  int pwm = int(fabsf(volts) / battery * MOTOR_PWM_MAX);
  if (pwm > MOTOR_PWM_MAX) pwm = MOTOR_PWM_MAX;
  digitalWrite(in1, volts >= 0 ? HIGH : LOW);
  digitalWrite(in2, volts >= 0 ? LOW : HIGH);
  pwmTimer->setCaptureCompare(channel, pwm, RESOLUTION_10B_COMPARE_FORMAT);
}

void setVolts(float left, float right, float battery) {
  left = constrain(left, -MAX_MOTOR_VOLTS, MAX_MOTOR_VOLTS);
  right = constrain(right, -MAX_MOTOR_VOLTS, MAX_MOTOR_VOLTS);
  drive(1, PIN_LEFT_IN1, PIN_LEFT_IN2, left, battery, LEFT_MOTOR_REVERSED);
  drive(2, PIN_RIGHT_IN1, PIN_RIGHT_IN2, right, battery, RIGHT_MOTOR_REVERSED);
}

void coast() {
  if (!pwmTimer) return;
  pwmTimer->setCaptureCompare(1, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwmTimer->setCaptureCompare(2, 0, RESOLUTION_10B_COMPARE_FORMAT);
  digitalWrite(PIN_LEFT_IN1, LOW);
  digitalWrite(PIN_LEFT_IN2, LOW);
  digitalWrite(PIN_RIGHT_IN1, LOW);
  digitalWrite(PIN_RIGHT_IN2, LOW);
}

}  // namespace motors

// =============================================================================
//  IR SENSORS - doc toi (LED tat) va sang (LED bat), lay hieu de loai anh sang moi truong
// =============================================================================
namespace sensors {

volatile float lss = 0, lfs = 0, rfs = 0, rss = 0;
volatile int rawLss = 0, rawLfs = 0, rawRfs = 0, rawRss = 0;
volatile float batteryVolts = 0;
static volatile bool enabled = false;

void begin() {
  pinMode(PIN_EMITTER_SIDE, OUTPUT);
  pinMode(PIN_EMITTER_FRONT, OUTPUT);
  digitalWrite(PIN_EMITTER_SIDE, LOW);
  digitalWrite(PIN_EMITTER_FRONT, LOW);
  analogReadResolution(12);
}

void setEnabled(bool on) { enabled = on; }

void update() {
  // Pin: doc 50 lan/giay, loc thong thap
  static uint8_t batteryTick = 0;
  if (batteryVolts < 1.0f || ++batteryTick >= LOOP_FREQUENCY / 50) {
    batteryTick = 0;
    const float vb = analogRead(PIN_BATTERY) * (ADC_REF_VOLTS / 4095.0f) * BATTERY_DIVIDER;
    batteryVolts = (batteryVolts < 1.0f) ? vb : batteryVolts * 0.9f + vb * 0.1f;
  }

  if (!enabled) {
    lss = lfs = rfs = rss = 0;
    return;
  }
  const int dLss = analogRead(PIN_SENSOR_LSS);
  const int dLfs = analogRead(PIN_SENSOR_LFS);
  const int dRfs = analogRead(PIN_SENSOR_RFS);
  const int dRss = analogRead(PIN_SENSOR_RSS);
  digitalWrite(PIN_EMITTER_SIDE, HIGH);
  digitalWrite(PIN_EMITTER_FRONT, HIGH);
  delayMicroseconds(EMITTER_SETTLE_US);
  const int lLss = analogRead(PIN_SENSOR_LSS);
  const int lLfs = analogRead(PIN_SENSOR_LFS);
  const int lRfs = analogRead(PIN_SENSOR_RFS);
  const int lRss = analogRead(PIN_SENSOR_RSS);
  digitalWrite(PIN_EMITTER_SIDE, LOW);
  digitalWrite(PIN_EMITTER_FRONT, LOW);

  rawLss = max(0, lLss - dLss);
  rawLfs = max(0, lLfs - dLfs);
  rawRfs = max(0, lRfs - dRfs);
  rawRss = max(0, lRss - dRss);
  lss = rawLss * LSS_SCALE;
  lfs = rawLfs * LFS_SCALE;
  rfs = rawRfs * RFS_SCALE;
  rss = rawRss * RSS_SCALE;
}

bool leftWall() { return lss > LEFT_WALL_THRESHOLD; }
bool rightWall() { return rss > RIGHT_WALL_THRESHOLD; }
bool frontWall() { return (lfs + rfs) * 0.5f > FRONT_WALL_THRESHOLD; }
float frontSum() { return lfs + rfs; }

float crossTrackError() {
  const bool l = leftWall(), r = rightWall();
  if (l && r) return lss - rss;
  if (l) return 2.0f * (lss - SIDE_NOMINAL);
  if (r) return 2.0f * (SIDE_NOMINAL - rss);
  return 0.0f;
}

}  // namespace sensors

// =============================================================================
//  UI
// =============================================================================
namespace ui {

void begin() {
  pinMode(PIN_BUTTON_GO, INPUT_PULLUP);
  pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  led(false);
  digitalWrite(PIN_BUZZER, LOW);
}

bool goPressed() { return digitalRead(PIN_BUTTON_GO) == LOW; }
bool modePressed() { return digitalRead(PIN_BUTTON_MODE) == LOW; }

bool waitRelease(uint32_t pin) {
  delay(20);  // chong doi phim
  while (digitalRead(pin) == LOW) delay(5);
  delay(20);
  return true;
}

void led(bool on) { digitalWrite(PIN_LED, on ? LOW : HIGH); }

void beep(uint16_t ms) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void beeps(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    beep(60);
    delay(80);
  }
}

void blink(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    led(true);
    delay(120);
    led(false);
    delay(150);
  }
}

}  // namespace ui
