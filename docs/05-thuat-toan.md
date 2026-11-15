# Thuật toán (cách robot nghĩ)

## 1. Kiến trúc phần mềm

```
lib/MazeCore/    (C++ thuần, không phụ thuộc phần cứng, test được trên PC)
  maze.*         Bản đồ tường (1 byte/ô: 4 bit tường + 4 bit "đã biết"), Flood Fill BFS
  explorer.*     Thuật toán thăm dò + giao diện MouseIO
  planner.*      Dijkstra (heap): planner thăm dò (ô, hướng) và planner theo THỜI GIAN (ô, hướng, nước đi trước)
  turns.*        Hình học các kiểu rẽ slalom; tự tính before/after bằng tích phân số
  smooth_path.*  Dịch chuỗi ô -> chuyển động slalom/chéo (port từ Bulebule, GPL-3.0), kiểm tra khả thi, ước lượng thời gian
  run_defaults.h Bán kính rẽ mặc định cho ô 192 mm
src/             (firmware STM32)
  config.h       Toàn bộ thông số: kích thước mê cung, cơ khí, PID, cảm biến, tốc độ, chân
  hardware.*     Encoder (TIM3/TIM4), động cơ (TIM1 PWM), cảm biến IR, nút/LED/còi
  profile.h      Profile vận tốc hình thang
  motion.*       Vòng điều khiển 1 kHz (F411): PD + feed-forward + bám tâm + đường cong theo quãng đường
  robot.*        RobotIO (thăm dò có rẽ cong), bộ thực thi speed run slalom
  storage.*      Lưu bản đồ vào Flash
sim/             Adapter cho trình mô phỏng mms
test/host/       test_core (thăm dò) + test_speedrun (slalom, mô phỏng động học, khe hở tường)
```

## 2. Thăm dò (Flood Fill)

1. Mỗi ô có *cost* = số ô tới đích, giả định **tường chưa biết là không có**.
2. Vào ô mới (tại "điểm cảm biến" = cạnh ô + `SENSING_OFFSET`) → đọc 3 tường → cập nhật bản đồ.
3. BFS lại cost (< 1 ms) → chọn ô kề cost nhỏ nhất (ưu tiên thẳng > phải > trái > quay đầu).
4. **Rẽ khi thăm dò dùng đường cong SS90, không dừng lại**. Chỉ dừng khi quay đầu (ngõ cụt) hoặc tới đích. Nếu `SENSING_OFFSET` không nhỏ hơn đoạn `before` của SS90 (~18 mm) thì firmware tự lùi về kiểu cũ: dừng ở tâm ô rồi quay tại chỗ.
5. Tới đích thì đồng hồ dừng. Robot **lưu bản đồ ngay** (phòng khi bị nhấc lên), rồi tiếp tục thăm các ô chưa đi trên đường lạc quan tốt nhất (`exploreUntilSolved`). Thăm dò dừng khi bản đồ đã đủ để chạy tối ưu, hoặc khi đi thêm `EXPLORE_MAX_CELLS` ô, hoặc khi hết `SEARCH_TIME_LIMIT_MS` (150 s kể từ lúc xuất phát). Sau đó robot về Start và lưu lại bản đồ.

Test 300 mê cung: 300/300 tới đích, **300/300 tìm ra đường tối ưu**, trung bình 82 ô tới đích và 163 ô tổng. Mê cung mẫu của BTC (`mazes/mysterious_maze_2026.txt`): 73 ô tới đích, 246 ô tổng. Lúc đầu giới hạn tính theo "số chặng" làm robot dừng ở 204 ô với đường dài hơn tối ưu 36%; nay đã sửa sang giới hạn theo số ô.

### 2.1 Bản đồ gợi ý (SEARCH+MAP)

Khi có bản đồ nạp sẵn, Flood Fill dùng chế độ `HINT`: tường **chưa đo** lấy theo bản đồ gợi ý thay vì coi là trống. Robot đi thẳng theo đường tối ưu của bản đồ gợi ý nhưng vẫn đo mọi tường. Tường đo được khác gợi ý thì số đo thật được ưu tiên và đường được tính lại. Lệch quá `PRESET_MISMATCH_TOLERANCE`, hoặc gợi ý chặn hết đường, thì gợi ý bị bỏ. Speed run luôn chỉ dùng tường đã đo. Chi tiết và số liệu ở [07-nap-ban-do-san.md](07-nap-ban-do-san.md).

## 3. Speed run tối ưu thời gian

### 3.1 Các kiểu rẽ (slalom)

| Kiểu | Chuỗi ô | Mô tả | R (mm) | before / after (mm) | v (m/s) @ 9 m/s² |
|---|---|---|---|---|---|
| SS90 | `L` | 90° gọn trong 1 ô (thăm dò, zig-zag khi không chạy chéo) | 52.5 | 18 / 18 | 0.69 |
| F90 | `F L F` | 90° bán kính lớn, ăn vào 2 đoạn thẳng | 138.7 | −67 / −67 | 1.12 |
| F180 | `F L L F` | Quay đầu qua 2 ô | 94.7 (tự tính) | 0 / 0 | 0.92 |
| D45 vào/ra | `F L R…` / `…L F` | Thẳng ↔ chéo 45° | 106.7 | −68 / +68 | 0.98 |
| D135 vào/ra | `F L L R…` / `…L L F` | Thẳng ↔ chéo 135° | 85.3 | −41 / +39 | 0.88 |
| V90 | `…L L R…` | Chéo → chéo 90° | 69.3 | 42 / 42 | 0.79 |
| Chéo | `…L R L R…` | Mỗi ô zig-zag = 1 bước chéo 135.8 mm | | | |

- **Độ cong biến thiên theo quãng đường**, không theo thời gian: tăng hình sin trong đoạn chuyển tiếp, giữ đều, rồi giảm hình sin. Robot không bị giật góc, và quỹ đạo không phụ thuộc tốc độ thực, nên không cần chỉnh lại khi đổi tốc độ. Ý tưởng lấy từ Bulebule.
- **before/after được tính tự động** (`turns.cpp`): tích phân số quỹ đạo, rồi giải để điểm vào và điểm ra trùng trung điểm cạnh ô. Test kiểm tra lại sai số < 0.5 mm. Bulebule dùng bảng số đo tay cho ô 180 mm, còn ở đây đổi kích thước ô hay bán kính là đúng ngay.
- **Tốc độ rẽ** = √(gia tốc hướng tâm × R), giới hạn bởi độ bám bánh (`lateralAccel`).

### 3.2 Dịch đường đi

Bộ từ điển hai trạng thái (THẲNG / CHÉO) được port từ Bulebule `path.c`. Kết quả dịch được đối chiếu với toàn bộ test `test_path.py` gốc của họ, kể cả chuỗi "Challenge" dài, và **khớp 100%**. Phần mở rộng:
- `S` (dừng ở đích) được coi như `F` khi nhìn trước.
- Kiểm tra khả thi: không cho đoạn thẳng âm, và phải đủ quãng phanh trước mỗi đường cong. Nếu không khả thi thì tự hạ xuống ngôn ngữ an toàn hơn: CHÉO → SAFE (F90/F180) → SEARCH (chỉ SS90).
- Chạy vào sâu thêm 1 ô trong vùng đích 2×2 nếu thông: đồng hồ dừng khi chạm Goal, nên phanh muộn hơn sẽ có lợi.

### 3.3 Planner theo thời gian

Dijkstra trên 3072 trạng thái (ô × hướng × nước đi ở ô trước: F/L/R). Cost là **mili-giây** tính từ cấu hình chạy:
- thẳng sau thẳng: 2h / v_top; thẳng ngay sau khi rẽ: chậm hơn (đang tăng tốc)
- rẽ sau thẳng: thời gian F90
- rẽ ngược chiều lần rẽ trước (zig-zag): 1 bước chéo ở `diagSpeed`
- rẽ cùng chiều liên tiếp: V90 / F180

Planner sinh nhiều ứng viên (mỗi ngôn ngữ + planner ít-rẽ), dịch từng cái, **ước lượng thời gian thật** và chọn đường nhanh nhất. Toàn bộ dùng heap nên tính trong vài ms.

### 3.4 Kết quả mô phỏng (300 mê cung, `test_speedrun.exe`)

Cùng cấu hình tốc độ (thẳng 2.0 m/s, chéo 1.5 m/s, 5 m/s², hướng tâm 9 m/s²):

| Kiểu chạy | Thời gian TB ước lượng |
|---|---|
| Chỉ SS90 | 8.07 s |
| + F90 / F180 | 7.50 s |
| **+ chạy chéo** | **5.79 s (nhanh hơn 28%)** |

Mô phỏng động học (robot chạy theo đúng đường cong thật trên mê cung thật, kiểm tra từng 2 mm):
- Điểm dừng cuối lệch < 1 mm.
- **Không chạm tường/cột trên cả 300 mê cung**. Khe hở nhỏ nhất (tại V90) là 12.3 mm với thân robot bán kính 45 mm, 7.3 mm với 50 mm, và từ ~60 mm thì bắt đầu chạm.

**⇒ Muốn chạy chéo, thân robot nên rộng ≤ 90–100 mm.** Kiểm tra với kích thước thật:

```bash
build/test_speedrun.exe 48
```

## 4. Chuyển động (firmware)

- **Hệ quy chiếu cạnh ô:** mọi lệnh nối tiếp nhau theo trung điểm cạnh ô; profile tích lũy nên sai số không dồn.
- **Vòng điều khiển 1 kHz (F411) / 500 Hz (F103):** PD trên sai số vị trí tích lũy + feed-forward vận tốc/gia tốc từng bánh, chia cho điện áp pin.
- **Đường cong slalom trong ngắt:** mỗi tick, góc đặt += độ_cong(s) × quãng đường vừa đi. Đường cong tự bắt đầu ngay khi đoạn thẳng trước kết thúc (tính cả vài mm robot đã trôi). Khi kết thúc, mốc tiến dịch đúng bằng độ dài đường cong.
- **Bám tâm đường** chỉ bật trên đoạn thẳng trực giao; tắt khi chạy chéo và khi đang cong.
- **An toàn:** sai số > 40 mm hoặc > 30° thì ngắt động cơ. Bấm GO khi đang chạy để dừng khẩn.

## 5. Hướng cải tiến tiếp

- Đọc ADC bằng DMA thay `analogRead` trong ngắt, giảm tải CPU ở 1 kHz.
- IMU (gyro) để đo góc chính xác khi rẽ tốc độ cao (encoder bị trượt).
- Phát hiện cột (post detection) để hiệu chỉnh vị trí dọc trước mỗi đường cong.
- Bám tường/cột bằng cảm biến trước khi chạy chéo.
- Giới hạn tốc độ theo ma sát thực tế (đo `lateralAccel` lớn nhất trước khi trượt).
