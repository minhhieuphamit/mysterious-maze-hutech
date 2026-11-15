# Phần cứng: chọn linh kiện, đi dây, lắp ráp, kiểm tra

Tài liệu này dẫn từ con số 0 tới một robot chạy được firmware trong project. Thông số mặc định trong `src/config.h` khớp với cấu hình **khuyên dùng** dưới đây. Nếu đổi linh kiện, xem cột "đổi gì trong config".

## 1. Sơ đồ khối

```
                 ┌──────────────┐
 LiPo 2S 7.4V ──►│ Công tắc + cầu chì 2A ├──┬──────────────► VM  TB6612FNG ──► 2 động cơ N20
                 └──────────────┘  │                         ▲ PWM/IN (PA8,PA9,PB14,PB15,PB5,PB10)
                                   ├──► Buck 5V ──► 5V BlackPill ──► LDO 3.3V (trên board)
                                   │                     │
                                   │                     ├──► Encoder N20 (3.3V) ──► PB6/PB7, PA6/PA7
                                   │                     ├──► 4 phototransistor ──► PA0, PA1, PA4, PA5
                                   │                     └──► Nút, LED, còi, UART debug
                                   ├──► LED IR (qua MOSFET PB12 / PB13)
                                   └──► Phân áp 20k/10k ──► PB0 (đo pin)
```

## 2. Danh sách linh kiện (BOM)

| # | Linh kiện | Thông số khuyên dùng | SL | Đổi gì trong `config.h` nếu khác |
|---|---|---|---|---|
| 1 | Vi điều khiển | **STM32F411CEU6 "BlackPill"** (100 MHz, FPU, 128 KB RAM) | 1 (+1 dự phòng) | BluePill F103 cũng build được nhưng RAM 88%, tắt planner theo thời gian |
| 2 | Động cơ | **N20 6V tỉ số 1:20 (~1500 rpm), có encoder từ 7 xung/vòng** | 2 | `GEAR_RATIO`, `ENCODER_PULSES` |
| 3 | Bánh xe | Bánh cao su/silicone Ø34 mm, trục D 3 mm cho N20 | 2 | `WHEEL_DIAMETER` |
| 4 | Driver động cơ | **TB6612FNG** (module 2 kênh, 1.2 A liên tục) | 1 | — |
| 5 | LED phát IR | 940 nm góc hẹp: SFH4545, TSAL6200, hoặc IR333 | 4 | — |
| 6 | Thu IR | Phototransistor 940 nm: TEFT4300, SFH309FA, hoặc PT333 | 4 | `*_SCALE`, ngưỡng tường (calib) |
| 7 | MOSFET N logic-level | AO3400 (SOT-23) hoặc 2N7002 | 2 | — |
| 8 | Điện trở | 33–47 Ω 0.25 W (LED IR) ×2; 10k–22k (phototransistor) ×4; 20k + 10k (đo pin); 100 Ω (gate) ×2 | | `BATTERY_DIVIDER` |
| 9 | Pin | **LiPo 2S 7.4 V 300–450 mAh**, xả ≥ 20C, jack JST/XT30 | 2 | `BATTERY_LOW_VOLTS` |
| 10 | Nguồn 5V | Buck mini 5V ≥ 1 A (MP1584 / Mini-360) | 1 | — |
| 11 | Bảo vệ | Công tắc gạt, cầu chì tự phục hồi 2 A, tụ 100 µF gần TB6612 | | — |
| 12 | Giao diện | 2 nút nhấn 6×6, còi **chủ động** 3.3–5 V, LED trên board | | — |
| 13 | Bi đỡ | Ball caster mini (bi thép 6–8 mm) hoặc 2 chân trượt Teflon | 1–2 | — |
| 14 | Khung | **PCB 2 lớp** làm luôn thân xe (khuyên dùng), hoặc mica/FR4 2 mm | 1 | — |
| 15 | Dụng cụ | ST-Link V2, USB-UART CP2102/CH340 (3.3 V), sạc LiPo cân bằng | | — |

### Chọn tỉ số truyền động cơ

Tốc độ tối đa (không tải) ≈ rpm / 60 × π × D. Với bánh 34 mm (số rpm tham khảo của N20 6V, kiểm tra datasheet của shop):

| N20 6V | rpm không tải | v tối đa | Ghi chú |
|---|---|---|---|
| 1:10 | ~3000 | ~5.3 m/s | Rất nhanh nhưng yếu, khó điều khiển ở tốc độ thấp |
| **1:20** | **~1500** | **~2.7 m/s** | **Khuyên dùng**: đủ cho RUN3 (2.2 m/s) |
| 1:30 | ~1000 | ~1.8 m/s | Chỉ chạy tốt tới ~1.3 m/s: hạ `topSpeed` của RUN2/RUN3 |
| 1:50 | ~600 | ~1.1 m/s | Quá chậm cho speed run |

Firmware giới hạn điện áp động cơ ở `MAX_MOTOR_VOLTS = 6 V`, kể cả khi pin 8.4 V.

### Khối lượng mục tiêu

Càng nhẹ càng tăng tốc và vào cua nhanh. Mục tiêu **≤ 120 g**: pin ~25 g, 2 động cơ + bánh ~25 g, PCB + linh kiện ~40 g, BlackPill ~8 g.

## 3. Sơ đồ chân (chung cho F411 BlackPill và F103 BluePill)

| Chức năng | Chân MCU | Nối tới | Ghi chú |
|---|---|---|---|
| Encoder trái A / B | **PB6 / PB7** | Encoder động cơ trái | TIM4 chế độ encoder (đếm phần cứng) |
| Encoder phải A / B | **PA6 / PA7** | Encoder động cơ phải | TIM3 chế độ encoder |
| PWM trái / phải | **PA8 / PA9** | TB6612 PWMA / PWMB | TIM1 CH1/CH2, 20 kHz |
| Chiều trái | **PB14 / PB15** | TB6612 AIN1 / AIN2 | |
| Chiều phải | **PB5 / PB10** | TB6612 BIN1 / BIN2 | |
| TB6612 STBY | — | 3.3 V | Luôn bật |
| IR trái cạnh | **PA0** | Phototransistor LSS | BlackPill: nút KEY cũng nối PA0, **đừng bấm khi chạy** |
| IR trái trước | **PA1** | Phototransistor LFS | |
| IR phải trước | **PA4** | Phototransistor RFS | |
| IR phải cạnh | **PA5** | Phototransistor RSS | |
| LED IR cạnh | **PB12** | Gate MOSFET nhóm cạnh | |
| LED IR trước | **PB13** | Gate MOSFET nhóm trước | |
| Đo pin | **PB0** | Giữa phân áp 20k/10k | |
| Nút GO / MODE | **PB8 / PB9** | Nút → GND | Kéo lên nội |
| Còi | **PB1** | Còi chủ động (qua transistor nếu còi > 20 mA) | HIGH = kêu |
| LED | PC13 | LED trên board | Sáng khi LOW |
| UART debug TX / RX | **PA2 / PA3** | RX / TX của USB-UART | 115200, **chỉ cắm khi ở ngoài sa bàn** |
| Nạp code | PA13 / PA14 | ST-Link SWDIO / SWCLK (+GND, 3.3V) | |

Đổi chân thì sửa mục 6 trong `config.h`. Encoder **bắt buộc** nằm trên cặp chân CH1/CH2 của TIM3/TIM4.

## 4. Mạch chi tiết

### 4.1 Nguồn

```
LiPo 2S (+) ──[Công tắc]──[Cầu chì 2A]──┬──────────────────── TB6612 VM  (+ tụ 100µF + 0.1µF sát chân)
                                       ├──[Buck 5V]────────── BlackPill 5V   (LDO trên board → 3.3V)
                                       ├──[20k]──┬──[10k]──── GND
                                       │         └──────────── PB0
                                       └──────────────────────── LED IR (xem 4.4)
LiPo (−) ─────────────────────────────────────────────────── GND chung (MỌI GND nối về một điểm)
```

- **Không** cấp pin 8.4 V thẳng vào chân 5V hoặc 3.3V của BlackPill.
- Khi đang cắm ST-Link/USB-UART: chỉ nối GND + tín hiệu, **không** nối 3.3V của dongle nếu robot đang bật pin.

### 4.2 TB6612FNG

| TB6612 | Nối | | TB6612 | Nối |
|---|---|---|---|---|
| VM | Pin (sau cầu chì) | | PWMA | PA8 |
| VCC | 3.3 V | | AIN1 / AIN2 | PB14 / PB15 |
| GND (3 chân) | GND | | PWMB | PA9 |
| STBY | 3.3 V | | BIN1 / BIN2 | PB5 / PB10 |
| AO1 / AO2 | Động cơ trái | | BO1 / BO2 | Động cơ phải |

Động cơ quay ngược chiều mong muốn thì **không cần hàn lại**: đổi `LEFT/RIGHT_MOTOR_REVERSED` trong config.

### 4.3 Encoder N20

Encoder từ N20 có 6 dây: M1, M2 (động cơ), VCC, GND, C1 (A), C2 (B).
- **VCC encoder = 3.3 V** để tín hiệu ra không vượt 3.3 V.
- C1/C2 → PB6/PB7 (trái), PA6/PA7 (phải). Firmware bật pull-up nội.
- Đếm sai chiều thì đổi `*_ENCODER_REVERSED`.

### 4.4 Cảm biến tường IR (×4)

**Phát (2 nhóm: cạnh và trước):** mỗi nhóm 2 LED mắc nối tiếp, bật bằng 1 MOSFET.

```
Pin 7.4–8.4V ──[R 39Ω]──►|──►|──┐         (2 LED IR nối tiếp ≈ 3.0 V)
                                │ D
PB12 (hoặc PB13) ──[100Ω]────── G   AO3400
                                │ S
                               GND
```

Dòng ≈ (8.4 − 3.0) / 39 ≈ 140 mA. Mỗi lần LED chỉ sáng ~50 µs trong mỗi ms (~5%), nên an toàn và tầm xa tốt.

**Thu (mắc emitter-follower: càng sáng → điện áp ADC càng cao):**

```
3.3V ──── C  Phototransistor
          E ──┬────────── PA0 / PA1 / PA4 / PA5
              │
             [R 10k–22k]
              │
             GND
```

> ⚠️ **Không** mắc kiểu điện trở kéo lên 3.3 V với transistor xuống GND. Kiểu đó đảo chiều tín hiệu (càng sáng càng thấp), mà firmware tính `sáng − tối`, nên sẽ ra 0.

Chọn điện trở sao cho khi robot đứng giữa ô, cảm biến cạnh đọc ~1500–2500/4095 (dùng mode SENSORS). Quá thấp thì tăng R, bão hòa (~4000) thì giảm R.

**Bố trí:**

```
            LFS ↑        ↑ RFS          (2 cảm biến trước: song song trục xe, cách nhau ~30–40 mm)
         LSS ↖  ┌────────┐  ↗ RSS       (2 cảm biến cạnh: nghiêng ~45° ra ngoài-về trước)
                │  MCU   │
             ●══╪════════╪══●           (trục bánh = tâm robot)
                │  Pin   │
                └────────┘
                    ○ bi đỡ
```

- Đặt cảm biến ở **đầu xe**, cách trục bánh ~35–45 mm về phía trước, cao ~20–30 mm so với sàn (tường cao 100 mm).
- Mỗi cặp LED + phototransistor đặt sát nhau, có **vách ngăn** (ống co nhiệt đen) để ánh sáng không lọt thẳng từ LED sang phototransistor.
- Cảm biến cạnh nghiêng về trước để tại "điểm đọc cảm biến" (vừa qua cạnh ô 8 mm, `SENSING_OFFSET`) đã nhìn thấy tường của ô mới.

### 4.5 Nút, còi, UART

- Nút GO/MODE: một chân nối PB8/PB9, chân kia nối GND (không cần điện trở).
- Còi chủ động: dưới 20 mA có thể nối thẳng PB1; lớn hơn thì dùng NPN (S8050) + điện trở 1k.
- UART: PA2 (TX) → RX của USB-UART, PA3 (RX) ← TX, nối GND chung.

## 5. Cơ khí

### Kích thước

| Thông số | Giới hạn / khuyên dùng | Lý do |
|---|---|---|
| Dài × rộng | **Luật: ≤ 150 × 150 mm** | Thể lệ |
| Bề rộng thân | **≤ 90–100 mm** | Chạy chéo: mô phỏng khe hở 12 mm với nửa bề rộng 45 mm, 7 mm với 50 mm |
| Bán kính bao (từ trục bánh tới góc xa nhất) | ≤ 75 mm | Quay tại chỗ trong ô 180 mm không quẹt tường |
| Khoảng cách 2 bánh | 70–80 mm | Ghi vào `WHEEL_SEPARATION`, calib lại sau |
| Mặt sau → trục bánh | Đo thật, ghi vào `BACK_WALL_TO_CENTER` | Robot xuất phát lưng chạm tường |

Kiểm tra bề rộng thật trên máy (tham số = nửa bề rộng, mm):

```bash
build/test_speedrun.exe 48
```

### Nguyên tắc

- **Trục bánh đi qua tâm robot** để quay tại chỗ không dịch chuyển.
- **Trọng tâm thấp và nằm gần trục bánh** (pin đặt ngay trên/giữa hai bánh). Bi đỡ chỉ đỡ nhẹ, khe hở sàn ~0.5 mm. Bi đè nặng sẽ làm bánh trượt khi tăng tốc.
- Bánh **cao su mềm, sạch**: lau bằng băng dính trước mỗi lượt (sàn có bụi là trượt khi rẽ).
- Động cơ bắt chặt bằng giá kẹp, không lắc. Encoder không chạm dây.
- Dây gọn và cố định, không quệt tường, không vướng bánh.

## 6. Lắp ráp từng bước

1. **Hàn nguồn trước:** công tắc, cầu chì, buck 5V. Cắm pin và đo: ra 5.0 V. **Chưa** gắn BlackPill.
2. **Gắn BlackPill** (qua header cái để dễ thay). Cấp nguồn: LED nguồn sáng, chân 3.3V đo ~3.3 V.
3. **Nạp thử firmware** qua ST-Link (xem [03-firmware-nap-va-van-hanh.md](03-firmware-nap-va-van-hanh.md)), mở UART: phải thấy `=== MYSTERIOUS MAZE 2026 ===` và điện áp pin đúng (sai thì kiểm tra phân áp và `BATTERY_DIVIDER`).
4. **Hàn TB6612 + động cơ.** Kê robot lên (bánh không chạm sàn), chạy mode STRAIGHT4: hai bánh phải quay cùng chiều tiến.
5. **Nối encoder.** Mode SENSORS, lăn robot tiến bằng tay → hai số `enc` phải **cùng tăng**.
6. **Hàn cảm biến IR.** Mode SENSORS: đưa tay/tờ giấy trắng lại gần từng cảm biến, giá trị `raw` tương ứng phải tăng; che sáng phòng mà giá trị không đổi nghĩa là khử ánh sáng nền đã hoạt động.
7. **Nút, còi:** bấm MODE thì LED nháy và in tên chế độ ra UART.
8. Cân robot, đo kích thước, đo `BACK_WALL_TO_CENTER` và khoảng cách bánh, ghi vào config.

## 7. Kiểm tra trước khi cấp nguồn lần đầu (bắt buộc)

- [ ] Đồng hồ thang điện trở giữa (+) pin và GND: **không** được ~0 Ω (chập).
- [ ] Cực pin đúng (đỏ +). Ngược cực sẽ cháy buck và driver.
- [ ] Không có chân nào của BlackPill nhận > 3.3 V (trừ chân 5V).
- [ ] Encoder cấp 3.3 V, **không** cấp từ pin.
- [ ] Phân áp pin: điện áp tại PB0 khi pin đầy ≤ 3.0 V.

## 8. Lỗi thường gặp

| Hiện tượng | Nguyên nhân hay gặp |
|---|---|
| Không nạp được code | Thiếu driver ST-Link, dây SWD lỏng, chưa cắm GND chung, BOOT0 đang ở 1 |
| Robot chạy rồi tự dừng ngay | Phát hiện "va chạm" do sai chiều động cơ/encoder → kiểm tra bước 4–5 |
| Robot quay vòng tại chỗ khi đi thẳng | Một bên đảo chiều encoder **hoặc** động cơ (không phải cả hai) |
| Cảm biến luôn = 0 | Mắc phototransistor kiểu kéo lên (xem 4.4), hoặc MOSFET không bật |
| Cảm biến nhiễu khi ánh sáng đổi | Thiếu vách ngăn quang, LED quá yếu |
| Pin báo sai | Sai tỉ lệ phân áp so với `BATTERY_DIVIDER` |
| Reset khi tăng tốc mạnh | Sụt áp: thiếu tụ gần TB6612, pin yếu, dây nguồn mảnh |
