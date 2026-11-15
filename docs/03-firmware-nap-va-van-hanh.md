# Firmware: build, nạp, vận hành robot

## 1. Cài đặt (một lần)

1. Python 3 (đã có), rồi:

```bash
python -m pip install platformio ziglang
```

2. **Driver ST-Link** cho Windows: tải gói "STSW-LINK009" từ trang ST và cài đặt. Lần build đầu tiên, PlatformIO tự tải toolchain STM32 (~200–300 MB).
3. Driver cho USB-UART (CP210x hoặc CH340) nếu Windows chưa tự nhận.

Gợi ý: cài VS Code + extension **PlatformIO IDE** để có nút Build/Upload/Monitor trên thanh trạng thái. Các lệnh dưới đây làm được việc tương tự từ terminal.

## 2. Build và nạp

Mọi lệnh chạy trong thư mục `D:\Competition\mysterious-maze`.

| Việc | Lệnh |
|---|---|
| Build cho BlackPill F411 (mặc định) | `python -m platformio run` |
| Build cho BluePill F103 | `python -m platformio run -e bluepill_f103c8` |
| Nạp qua ST-Link | `python -m platformio run -t upload` |
| Xem log UART | `python -m platformio device monitor -b 115200` |

Nối ST-Link: **SWDIO ↔ PA13 (DIO), SWCLK ↔ PA14 (CLK), GND ↔ GND**. Nếu robot đang bật pin thì không nối 3.3V của ST-Link.

Sau khi nạp, log UART in ra:

```
=== MYSTERIOUS MAZE 2026 ===
Hinh hoc re: OK
Tham do: re cong SS90
Pin: 8.12
```

- `Hinh hoc re: LOI`: bảng bán kính rẽ (`run_defaults.h`) sai. Chạy `test_speedrun.exe` để xem chi tiết.
- `Tham do: dung + quay tai cho`: `SENSING_OFFSET` lớn hơn đoạn `before` của SS90 (~18 mm), nên firmware tự lùi về kiểu rẽ an toàn.

**Trước mỗi lần nạp**, nên chạy test trên máy ([06-test-tren-may.md](06-test-tren-may.md)) để chắc thuật toán không hỏng:

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```

## 3. Điều khiển robot

| Thao tác | Tác dụng |
|---|---|
| Bật nguồn | Kêu 1 tiếng, LED nháy N lần = chế độ hiện tại (N = số thứ tự) |
| Bấm **MODE** | Sang chế độ tiếp theo, LED nháy N lần, UART in tên chế độ |
| Bấm **GO** | Bắt đầu chế độ đang chọn (kêu 1 tiếng dài) |
| Chế độ chạy: **đưa tay trước cảm biến trước rồi rút ra** | Kêu "bíp", 0.5 s sau robot xuất phát. Tay không chạm robot nên robot không bị xê dịch |
| Bấm **GO khi robot đang chạy** | Dừng khẩn cấp (ngắt động cơ, bản đồ đã biết vẫn được lưu) |

### Các chế độ

| # | Tên | Làm gì | Khi nào dùng |
|---|---|---|---|
| 1 | **SEARCH** | Dò tới đích (rẽ cong, không dừng), lưu bản đồ, dò thêm tới khi chắc có đường tối ưu, về Start, lưu lại | Lượt đầu ở mê cung mới |
| 2 | **SEARCH+MAP** | Như SEARCH nhưng dùng bản đồ nạp sẵn (`src/preset_maze.h`) làm gợi ý | Khi có bản đồ dự đoán **và BTC cho phép** ([07](07-nap-ban-do-san.md)) |
| 3 | **RUN1 SS90** | Speed run theo bản đồ đã lưu: chỉ rẽ nhỏ, 1.0 m/s | Lượt chạy nhanh đầu tiên / an toàn |
| 4 | **RUN2 F90** | Thêm cua lớn F90/F180, 1.6 m/s | Khi RUN1 ổn |
| 5 | **RUN3 DIAG** | Chạy chéo 45°/135°/V90, 2.2 m/s | Khi RUN2 ổn, nhanh nhất |
| 6 | SENSORS | In IR thô/chuẩn hóa, có tường hay không, encoder, pin (10 lần/giây). Bấm GO/MODE để thoát | Calib, kiểm tra trước lượt |
| 7 | STRAIGHT4 | Chạy thẳng ~818 mm rồi dừng, in quãng đường encoder | Calib đường kính bánh |
| 8 | SPIN 720 | Quay tại chỗ 2 vòng, in góc encoder | Calib khoảng cách bánh |
| 9 | PRINT MAP | In bản đồ đã lưu dạng chữ | Kiểm tra robot đã biết gì |
| 10 | CLEAR MAP | Xóa bản đồ đã lưu | **Bắt buộc khi sang mê cung mới** |

Chế độ RUN tự chuyển sang SEARCH nếu chưa có bản đồ, hoặc bản đồ chưa có đường đã kiểm chứng tới đích.

Pin dưới `BATTERY_LOW_VOLTS` (6.8 V) thì SEARCH/RUN từ chối chạy và kêu 5 tiếng.

### Âm báo

| Âm | Ý nghĩa |
|---|---|
| 2 tiếng | Đã tới đích (đã lưu bản đồ) |
| 3 tiếng | Xong. SEARCH: bản đồ đã tối ưu. RUN: chạy xong |
| 1 tiếng (cuối SEARCH) | Đã lưu bản đồ nhưng **chưa chắc** tối ưu (hết giờ / hết ô) |
| 4 tiếng | Không tới được đích (hủy, va chạm, hết giờ, không có đường) |
| 5 tiếng | Speed run lỗi, hoặc pin yếu |

## 4. Bản đồ lưu trong Flash

- Bản đồ được lưu **lúc tới đích** và **khi kết thúc SEARCH**. Tắt nguồn vẫn còn.
- SEARCH **nối tiếp** bản đồ cũ nếu có. Vì vậy sang mê cung mới phải chạy **CLEAR MAP** trước.
- RUN chỉ dùng **tường robot đã tự đo**. Bản đồ nạp sẵn (SEARCH+MAP) không được dùng cho speed run.

## 5. Đọc log UART sau lượt chạy

SEARCH:

```
Toi dich sau (ms): 41230
Ban do TOI UU, da luu.
```

RUN:

```
Plan (ms): 12
Duong: BFFFFFFFFFFFRLRLRLR...S
Slalom: START F F ... D45inR D D D45outR ... STOP
Uoc luong (ms): 7240  muc ngon ngu: 2
Speed run xong (ms): 7810
```

- `Uoc luong`: thời gian lý tưởng theo mô hình. Thời gian thật thường chậm hơn 5–15%. Chênh nhiều hơn thì feed-forward/PID chưa chuẩn ([04-calib-va-tinh-chinh.md](04-calib-va-tinh-chinh.md)).
- `muc ngon ngu`: 0 = chỉ SS90, 1 = có F90/F180, 2 = có chạy chéo. Thấp hơn mức mong muốn nghĩa là planner thấy đường chéo không khả thi (đoạn thẳng quá ngắn để phanh) và đã tự hạ cấp.

## 6. Cấu trúc mã nguồn

```
src/config.h          MỌI thông số (xem tài liệu 04, mục "Tham chiếu config.h")
src/main.cpp          Menu chế độ, luồng SEARCH / RUN
src/robot.*           RobotIO (thăm dò trên robot thật), thực thi speed run slalom
src/motion.*          Vòng điều khiển 1 kHz: profile, PD, feed-forward, bám tường, đường cong
src/hardware.*        Encoder (TIM3/TIM4), PWM (TIM1), IR, pin, nút/LED/còi, UART
src/storage.*         Lưu/đọc bản đồ Flash (CRC)
src/preset_maze.h     Bản đồ nạp sẵn (tạo bằng scripts/maze_to_header.py)
lib/MazeCore/         Thuật toán thuần C++: dùng chung cho firmware, mô phỏng, test (tài liệu 05)
```
