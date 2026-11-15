# Chiến lược thi MYSTERIOUS MAZE 2026

## 1. Nguyên tắc

1. **Thành tích = thời gian từ lúc rời Start tới khi chạm Goal.** Thời gian thăm dò *sau* khi chạm Goal không bị tính, nhưng vẫn nằm trong giới hạn 3 phút của lượt.
2. **3 phút mỗi lượt tính cả thời gian chuẩn bị**: đặt robot, chọn chế độ, chỉnh tham số. Nhanh tay là có thêm thời gian cho robot.
3. **Không có lượt nào được phép "trắng tay" ở vòng loại.** Chỉ cần chạm Goal 1 lần trong 3 lượt, nên ưu tiên chắc chắn.
4. **Bản đồ là tài sản.** Robot lưu bản đồ vào Flash (lúc tới đích và khi kết thúc thăm dò), nên lượt sau chạy nhanh ngay mà không phải dò lại. Cần hỏi BTC xác nhận được phép (mục 6).
5. **Tăng tốc theo bậc, không nhảy cóc:** RUN1 → RUN2 → RUN3. Một lượt hỏng ở chung kết (chỉ có 2 lượt) là mất 50% cơ hội.

## 2. Số liệu trên mê cung mẫu của BTC (mô phỏng)

| Việc | Số ô | Thời gian ước tính* |
|---|---|---|
| SEARCH: tới đích | 73 | ~45 s |
| SEARCH: thăm dò đủ để chắc tối ưu + về Start | 246 | ~150 s |
| SEARCH+MAP (map đúng): tới đích | 57 | ~35 s |
| SEARCH+MAP (map đúng): đủ tối ưu + về Start | 120 | ~75 s |
| Quét **toàn bộ** mê cung + về Start | 488 | ~290 s, **vượt 3 phút** |

\*Thăm dò ở 0.35 m/s, ~0.6 s/ô, rẽ cong không dừng.

| Speed run (đường tối ưu đã biết) | Ước lượng lý tưởng | Thực tế dự kiến (+5–15%) |
|---|---|---|
| RUN1: chỉ cua nhỏ, 1.0 m/s | 15.8 s | ~17–18 s |
| RUN2: cua lớn F90/F180, 1.6 m/s | 10.4 s | ~11–12 s |
| RUN3: chạy chéo, 2.2 m/s | **7.0 s** | ~7.5–8 s |

Mê cung thật có thể khác hình dự kiến, nhưng thứ tự lớn nhỏ của các con số sẽ tương tự.

## 3. Kế hoạch từng vòng

### Vòng loại: 3 lượt, mục tiêu chạm Goal ít nhất 1 lần

| Lượt | Chế độ | Ghi chú |
|---|---|---|
| Trước lượt 1 | **CLEAR MAP** | Mê cung mới |
| 1 | **SEARCH** | Tới đích ~45 s là qua vòng. Robot tiếp tục dò và về Start nếu còn giờ. Bị nhấc lên giữa chừng cũng không sao: bản đồ đã được lưu lúc tới đích |
| 2 | **RUN1** (hoặc RUN2 nếu đã chạy ổn khi luyện) | Lấy thời gian tốt làm "mốc" an toàn. Không cần mạo hiểm |
| 3 | **RUN2** | Nếu lượt 2 ổn. Lượt 1 chưa tới đích thì lượt 3 chạy SEARCH lại (nối tiếp bản đồ cũ) |

### Vòng tuyển chọn: 3 lượt, top 8 thời gian ngắn nhất

Nếu mê cung **giữ nguyên** từ vòng loại và bản đồ đã tối ưu (lúc SEARCH kêu 3 tiếng):

| Lượt | Chế độ |
|---|---|
| 1 | **RUN2**: lấy thành tích chắc chắn |
| 2 | **RUN3** |
| 3 | RUN3 nếu lượt 2 ổn, ngược lại RUN2 |

Nếu mê cung **đổi**: CLEAR MAP → lượt 1 SEARCH → lượt 2 RUN2 → lượt 3 RUN3.

Nếu bản đồ **chưa tối ưu** (SEARCH kêu 1 tiếng): có thể dành lượt 1 cho SEARCH tiếp để hoàn thiện bản đồ, đổi lại mất 1 lượt chạy nhanh.

### Chung kết: chỉ 2 lượt

| Tình huống | Lượt 1 | Lượt 2 |
|---|---|---|
| Mê cung giữ nguyên, bản đồ tối ưu | **RUN2** (chắc chắn có thời gian) | **RUN3** (đánh cược) |
| Mê cung đổi | **SEARCH** (tính thời gian tới Goal) | **RUN2** (RUN3 chỉ khi đã chạy ổn định ở luyện tập) |

Lượt 1 đã có thời gian tốt thì lượt 2 mới mạo hiểm RUN3. Lượt 1 hỏng thì lượt 2 chạy **mức an toàn nhất còn lại**.

## 4. Quy trình trong 3 phút của mỗi lượt

Chia vai: **người điều khiển** (đặt robot, bấm nút), **người ghi chép** (ghi chế độ, thời gian, sự cố), **người theo dõi đồng hồ**.

1. **(0:00)** Đặt robot ở ô Start: **lưng chạm tường sau, nằm giữa ô, hướng Bắc**.
2. Chế độ đã được chọn **từ trước** khi tới lượt. Bấm MODE ngay từ lúc chờ, đếm số nháy LED.
3. Bấm **GO** → đưa tay trước cảm biến trước → rút tay ra. Robot xuất phát sau 0.5 s. **Không chạm robot.**
4. Robot chạm Goal → trọng tài ghi thời gian.
5. SEARCH: để robot tự dò và về Start nếu còn giờ (nhìn đồng hồ). Gần hết 3 phút thì robot tự dừng ở 150 s; cần thì bấm GO để dừng.
6. RUN: robot dừng trong vùng đích. Nhấc robot ra **sau khi trọng tài cho phép**.
7. Robot va chạm hoặc mất kiểm soát: bấm GO để dừng, đừng để quá 30 s đứng yên.

**Giữa các lượt:** lau bánh bằng băng dính, kiểm tra pin (mode SENSORS in điện áp; < 7.4 V thì thay pin), quyết định chế độ lượt sau.

## 5. Quản lý rủi ro

| Rủi ro | Phòng ngừa |
|---|---|
| Ánh sáng hội trường làm sai cảm biến | Lúc Technical Check / chạy thử: mode SENSORS trên sa bàn thật, chỉnh ngưỡng nếu cần (chỉnh phần mềm được phép) |
| Bánh trượt do bụi sàn | Lau bánh trước mỗi lượt; hạ `accel` / `lateralAccel` 20% so với lúc luyện tập |
| Pin yếu làm chậm và lệch | Sạc đầy mọi pin, thay pin giữa các vòng; firmware bù điện áp nhưng pin quá yếu sẽ sụt áp |
| Bản đồ cũ của mê cung khác còn trong Flash | **CLEAR MAP** mỗi khi đổi mê cung (ghi vào checklist) |
| Robot đâm tường ở RUN3 | Chỉ dùng RUN3 khi đã chạy ổn ≥ 3 lần trên mê cung luyện có đoạn chéo |
| Hết 3 phút khi đang thăm dò | `SEARCH_TIME_LIMIT_MS = 150 s`. Bản đồ được lưu ngay lúc tới đích |
| Firmware nạp nhầm bản thử | Đặt tên nhánh/phiên bản rõ ràng; trước ngày thi khóa một bản đã chạy ổn |

## 6. Câu hỏi cần xin BTC xác nhận trước

1. Sau khi chạm Goal, robot có được **chạy tiếp** (dò thêm, tự về Start) trong cùng lượt không?
2. Robot có được **giữ bản đồ trong bộ nhớ** giữa các lượt / các vòng không?
3. Mê cung có **giữ nguyên** giữa vòng loại, tuyển chọn, chung kết không?
4. Có được **nạp sẵn bản đồ dự kiến** (Hình 1) làm gợi ý không? (Robot vẫn tự đo tường.)
5. Robot được phép bắt đầu bằng cách **vẫy tay trước cảm biến** (không chạm robot) không?
6. Pin dự phòng cùng loại có được coi là thay "phần cứng chính" không?

| Trả lời | Ảnh hưởng chiến lược |
|---|---|
| (1) Không | SEARCH chỉ có tác dụng tới lúc chạm Goal → các lượt sau có thể cần SEARCH tiếp để hoàn thiện bản đồ |
| (2) Không | Mỗi lượt phải tự dò: dùng chế độ RUN (tự chuyển SEARCH khi chưa có bản đồ); thời gian sẽ là thời gian thăm dò |
| (4) Có | Lượt đầu dùng SEARCH+MAP: tiết kiệm ~75 s thăm dò, map sai thì robot tự bỏ |
| (4) Không | Chỉ dùng SEARCH; đặt `src/preset_maze.h` về rỗng (`python scripts/maze_to_header.py --none`) để tránh hiểu lầm |

## 7. Ghi chép sau mỗi lượt (mẫu)

| Vòng / lượt | Chế độ | Thời gian | Tới đích? | Sự cố | Tham số thay đổi |
|---|---|---|---|---|---|
| Loại / 1 | SEARCH | | | | |
| Loại / 2 | RUN1 | | | | |
| … | | | | | |
