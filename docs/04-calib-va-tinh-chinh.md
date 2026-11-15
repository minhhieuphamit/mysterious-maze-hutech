# Calib và tinh chỉnh trên robot thật

**Mọi số trong `src/config.h` chỉ là điểm khởi đầu.** Code đã qua biên dịch và mô phỏng nhưng chưa chạy trên robot của đội. Làm **đúng thứ tự** dưới đây; mỗi bước dựa vào bước trước. Sửa xong `config.h` thì nạp lại (`python -m platformio run -t upload`).

Cần: thước dây hoặc thước kẹp, 1 đoạn mê cung thử (ít nhất 2×4 ô có tường), USB-UART để xem log.

## Bước 1: Chiều động cơ và encoder

1. Mode **SENSORS**, lăn robot **tiến** bằng tay khoảng 20 cm.
   - Cả hai số `enc` phải **tăng**. Bên nào giảm thì đổi `LEFT_ENCODER_REVERSED` / `RIGHT_ENCODER_REVERSED`.
2. Kê robot lên (bánh không chạm sàn), mode **STRAIGHT4**.
   - Hai bánh phải quay theo chiều **tiến**. Bên nào ngược thì đổi `LEFT_MOTOR_REVERSED` / `RIGHT_MOTOR_REVERSED`.
   - Robot dừng ngay và kêu: thường do một bên đảo chiều động cơ mà không đảo encoder (hoặc ngược lại). Bộ phát hiện va chạm ngắt điện.

## Bước 2: Đường kính bánh (quãng đường)

1. Đặt robot lưng chạm tường trên sàn phẳng, mode **STRAIGHT4**. Robot chạy `START_TO_CENTER + 4 ô` = **818 mm** (với `BACK_WALL_TO_CENTER = 40`).
2. Đo quãng đường thật `A` (mm) robot đã đi.
3. `WHEEL_DIAMETER_mới = WHEEL_DIAMETER × A / 818`
4. Lặp lại tới khi sai số < 3 mm.

Robot đi lệch sang **trái** thì bánh phải đi xa hơn mỗi xung: tăng `RIGHT_WHEEL_ADJUST` thêm ~0.005 (hoặc giảm `LEFT_WHEEL_ADJUST`). Lệch phải thì làm ngược lại. Làm trên đoạn **không có tường** để bám tường không che mất lỗi.

## Bước 3: Khoảng cách bánh (góc quay)

1. Đánh dấu hướng robot, mode **SPIN 720**: robot quay 720°.
2. Đo góc thật `G` (độ). Ví dụ thiếu 10° thì G = 710.
3. `WHEEL_SEPARATION_mới = WHEEL_SEPARATION × 720 / G`
   - **Quay thiếu (G < 720) → tăng**, quay dư → giảm. Lý do: robot tính góc = (hiệu quãng đường 2 bánh) / `WHEEL_SEPARATION`; số trong config nhỏ hơn thực tế thì robot tưởng đã quay đủ trong khi thực tế còn thiếu.
4. Lặp tới khi sai < 2°. Bánh mòn hoặc bẩn sẽ làm số này đổi, nên kiểm tra lại trước ngày thi.

## Bước 4: Feed-forward (điện áp theo tốc độ)

Feed-forward giúp robot bám tốc độ đúng mà không phụ thuộc hết vào PID. Đây là bước quan trọng nhất cho chạy nhanh.

1. Tạm đặt: `FWD_KP = 0.02`, `FWD_KD = 0`, `ROT_KP = 0.02`, `ROT_KD = 0`, `FWD_ERROR_LIMIT = 1000`, `ROT_ERROR_LIMIT = 1000`.
2. **BIAS_FF:** tăng dần từ 0.1 V cho tới khi robot vừa đủ nhích được khi chạy STRAIGHT4 chậm. Thường 0.15–0.4 V.
3. **SPEED_FF:** chạy STRAIGHT4 và xem log `Encoder do duoc (mm)`:
   - Ngắn hơn 818 nhiều (robot không theo kịp): tăng `SPEED_FF`.
   - Vượt quá 818: giảm.
   - Giá trị lý thuyết ≈ 6 V / tốc độ không tải (mm/s). Với N20 1:20 khoảng 0.0022–0.0030.
4. **ACC_FF:** tăng `SEARCH_PARAMS.accel` lên cao (3000), xem robot có hụt ở đoạn tăng tốc không; hụt thì tăng `ACC_FF` (0.0003–0.001).
5. **Trả lại** giá trị PID và giới hạn lỗi ban đầu rồi sang bước 5.

## Bước 5: PD vị trí

1. Tăng `FWD_KP` tới khi đẩy nhẹ robot lúc đứng yên thấy "cứng" mà không rung. Rung thì giảm 30%.
2. Tăng `FWD_KD` để dập dao động. KD tính trên sai số **mỗi tick**, nên đổi `LOOP_FREQUENCY` thì phải chỉnh lại KD.
3. Làm tương tự `ROT_KP` / `ROT_KD` với SPIN 720: quay xong phải dừng gọn, không lắc qua lại.

## Bước 6: Cảm biến tường

Dùng mode **SENSORS**, đọc cột `raw` (giá trị thô) và `val` (đã chuẩn hóa).

| Đo ở đâu | Đọc gì | Ghi vào |
|---|---|---|
| Robot **ở tâm ô**, có tường hai bên | `raw` LSS, RSS | `LSS_SCALE = 100 / raw_LSS`, `RSS_SCALE = 100 / raw_RSS` |
| Robot **ở tâm ô**, quay mặt vào tường trước | `raw` LFS, RFS | `LFS_SCALE = 100 / raw_LFS`, `RFS_SCALE = 100 / raw_RFS` (khi đó `FRONT_REFERENCE = 200`) |
| Robot ở **điểm cảm biến** (mũi qua cạnh ô 8 mm), có tường cạnh | `val` LSS/RSS | `LEFT/RIGHT_WALL_THRESHOLD` ≈ 50% giá trị này |
| Như trên nhưng **không** có tường cạnh | `val` LSS/RSS | Phải < 30% ngưỡng (nếu không thì cảm biến bắt nhầm tường ô cũ: chỉnh góc cảm biến hoặc `SENSING_OFFSET`) |
| Robot ở điểm cảm biến, tường trước ở **cuối ô đó** | `(val LFS + val RFS)/2` | `FRONT_WALL_THRESHOLD` ≈ 50% giá trị này |
| Đưa tay cách cảm biến trước ~3 cm | `val LFS + RFS` | `START_HAND_THRESHOLD` ≈ 70% giá trị này |

Cột `wall L F R` phải đúng ở mọi vị trí thử. **Kiểm tra lại trên sa bàn thi** vì ánh sáng hội trường và màu tường có thể khác.

## Bước 7: Bám tâm đường

1. Đặt robot lệch sang một bên trong hành lang có 2 tường, chạy STRAIGHT4.
2. Tăng `STEERING_KP` tới khi robot tự về giữa trong ~1 ô. Lắc qua lại thì tăng `STEERING_KD` hoặc giảm KP.
3. `STEERING_MAX` giới hạn độ gắt hiệu chỉnh (deg/s).

## Bước 8: Thăm dò (SEARCH)

1. Dựng mê cung thử nhỏ, **CLEAR MAP**, chạy **SEARCH** ở `SEARCH_PARAMS.speed = 250`.
2. Sau mỗi lượt chạy **PRINT MAP**, so với mê cung thật: tường nào sai là cảm biến/ngưỡng sai.
3. Ổn rồi thì tăng `speed` lên 350 → 450 (nhanh hơn thì cần cảm biến rất ổn định).
4. Rẽ cong khi thăm dò bị văng ra ngoài thì giảm `speed` hoặc tăng `ROT_KP`. Không ổn thì tạm đặt `smoothTurns = false` (dừng và quay tại chỗ, chậm hơn nhưng chắc).

## Bước 9: Speed run, tăng dần

Lần lượt **RUN1 → RUN2 → RUN3**. Mỗi mức chạy ổn **3 lần liên tiếp** mới lên mức tiếp. Trong `RUN_PARAMS`:

| Tham số | Tăng lên được khi | Hạ xuống khi |
|---|---|---|
| `topSpeed` | Thẳng ổn, phanh kịp trước cua | Trượt bánh, chạy quá cua |
| `accel` | Bánh không trượt khi tăng tốc | Trượt hoặc vọt (bánh rít) |
| `lateralAccel` | Ra cua đúng giữa đường | Văng ra ngoài cua, quẹt tường |
| `diagSpeed` | Chạy chéo không quẹt cột | Quẹt cột ở đoạn chéo |

So `Uoc luong (ms)` và `Speed run xong (ms)` trong log: thật chậm hơn ước lượng > 20% thì feed-forward chưa chuẩn (bước 4).

## Bước 10: Bán kính cua (nâng cao)

Bảng bán kính nằm trong `lib/MazeCore/src/run_defaults.h`. Đoạn `before/after` **tự tính**, nên chỉ cần sửa bán kính. Sửa xong bắt buộc chạy:

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```

```bash
build/test_speedrun.exe 48
```

Số `48` là nửa bề rộng thân robot (mm). Khe hở nhỏ nhất phải > 5 mm. Bán kính lớn hơn cho tốc độ cua cao hơn, nhưng ăn vào đoạn thẳng nhiều hơn và sát cột hơn.

---

## Tham chiếu `config.h`

### Kích thước sa bàn và robot

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `CELL_INTERIOR`, `WALL_THICKNESS` | 180, 12 | Theo thể lệ BTC: bước ô = 192 mm |
| `BACK_WALL_TO_CENTER` | 40 | Mặt sau robot → trục bánh. **Đo thật** |
| `SENSING_OFFSET` | 8 | Đọc cảm biến khi qua cạnh ô X mm. Phải < 15 để rẽ cong khi thăm dò |
| `START_CELL_HAS_EAST_WALL` | true | Ô Start có tường phía Đông (như hình BTC) |

### Cơ khí và động cơ

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `WHEEL_DIAMETER` | 34 | Calib bước 2 |
| `ENCODER_PULSES`, `GEAR_RATIO` | 7, 20 | Theo động cơ (N20 1:20, encoder 7 xung) |
| `WHEEL_SEPARATION` | 74 | Calib bước 3 |
| `LEFT/RIGHT_WHEEL_ADJUST` | 1.0 | Bù chênh lệch 2 bánh |
| `*_MOTOR_REVERSED`, `*_ENCODER_REVERSED` | | Bước 1 |

### Điều khiển

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `LOOP_FREQUENCY` | 1000 (F411) / 500 (F103) | Tần số vòng điều khiển |
| `FWD_KP/KD`, `ROT_KP/KD` | 0.10/1.0, 0.07/0.8 | PD vị trí (bước 5) |
| `SPEED_FF`, `ACC_FF`, `BIAS_FF` | 0.0025, 0.0005, 0.2 | Feed-forward (bước 4) |
| `MAX_MOTOR_VOLTS` | 6.0 | Giới hạn áp động cơ |
| `FWD_ERROR_LIMIT`, `ROT_ERROR_LIMIT` | 40 mm, 30° | Vượt thì coi là va chạm, ngắt động cơ |

### Cảm biến

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `LSS/LFS/RFS/RSS_SCALE` | 0.1 | Chuẩn hóa về ~100 (bước 6) |
| `SIDE_NOMINAL`, `FRONT_REFERENCE` | 100, 200 | Giá trị khi robot ở tâm ô |
| `*_WALL_THRESHOLD` | 40, 40, 20 | Ngưỡng có tường |
| `STEERING_KP/KD/MAX` | 1.2, 4.0, 120 | Bám tâm đường (bước 7) |
| `ALIGN_*` | | Căn tường trước khi dừng ở tâm ô |
| `START_HAND_THRESHOLD` | 150 | Ngưỡng "vẫy tay" để xuất phát |
| `BATTERY_DIVIDER`, `BATTERY_LOW_VOLTS` | 3.0, 6.8 | Phân áp 20k/10k, ngưỡng pin yếu 2S |

### Chạy

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `SEARCH_PARAMS` | 350 mm/s, rẽ cong | Tốc độ thăm dò |
| `RUN_PARAMS[0..2]` | 1.0 / 1.6 / 2.2 m/s | RUN1 / RUN2 / RUN3 (bước 9) |
| `TURN_SPECS` | `kDefaultTurnSpecs` | Bảng bán kính cua (bước 10) |
| `EXPLORE_MAX_CELLS` | 300 | Số ô tối đa đi thêm sau khi tới đích |
| `SEARCH_TIME_LIMIT_MS` | 150000 | Tự dừng thăm dò sau 150 s từ lúc xuất phát |
| `PRESET_MISMATCH_TOLERANCE` | 4 | SEARCH+MAP: lệch quá số tường này thì bỏ bản đồ gợi ý |
