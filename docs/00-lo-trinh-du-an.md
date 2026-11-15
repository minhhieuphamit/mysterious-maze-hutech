# Lộ trình dự án: từ con số 0 tới ngày thi

Chia việc cho đội 3 người (gợi ý):

| Vai trò | Phụ trách | Tài liệu chính |
|---|---|---|
| **Phần cứng** | Linh kiện, mạch, PCB, lắp ráp, pin | [02-phan-cung.md](02-phan-cung.md) |
| **Firmware & calib** | Nạp code, calib, tinh chỉnh tốc độ | [03](03-firmware-nap-va-van-hanh.md), [04](04-calib-va-tinh-chinh.md) |
| **Thuật toán & chiến lược** | Test mô phỏng, mê cung luyện, chiến lược, ghi chép | [05](05-thuat-toan.md), [06](06-test-tren-may.md), [08](08-chien-luoc-thi.md) |

## Lộ trình 6 tuần (co giãn theo thời gian thực tế)

### Tuần 1: Chuẩn bị
- [ ] Đọc thể lệ ([01](01-tong-hop-the-le.md)), gửi câu hỏi cho BTC (mục 5 trong 01).
- [ ] Đặt mua linh kiện theo BOM ([02](02-phan-cung.md) mục 2), **mua dư** BlackPill, động cơ, cảm biến.
- [ ] Cài môi trường, chạy test trên máy: `scripts/build_host.ps1` ra `KET QUA: OK` ([06](06-test-tren-may.md)).
- [ ] Xem hình SVG mê cung BTC để hiểu robot sẽ chạy thế nào.
- [ ] Vẽ/đặt PCB hoặc khung (bề rộng ≤ 100 mm).

### Tuần 2: Lắp ráp và bring-up
- [ ] Lắp theo [02](02-phan-cung.md) mục 6, **kiểm tra chập trước khi cắm pin** (mục 7).
- [ ] Nạp firmware, đọc được log UART và điện áp pin ([03](03-firmware-nap-va-van-hanh.md)).
- [ ] Động cơ, encoder, cảm biến đều phản hồi đúng trong mode SENSORS.
- [ ] Đóng mê cung luyện tập (ít nhất 4×4 ô, đúng **lọt lòng 180 mm, tường 12 mm, cao 100 mm, trắng trên nền đen**).

### Tuần 3: Calib chuyển động
- [ ] [04](04-calib-va-tinh-chinh.md) bước 1–5: chiều, đường kính bánh, khoảng cách bánh, feed-forward, PD.
- [ ] Robot chạy thẳng 4 ô sai < 5 mm, quay 720° sai < 3°.

### Tuần 4: Cảm biến và thăm dò
- [ ] [04](04-calib-va-tinh-chinh.md) bước 6–8: ngưỡng tường, bám tâm, SEARCH.
- [ ] SEARCH trên mê cung luyện 10 lần liên tiếp, PRINT MAP **đúng 100%** số tường.
- [ ] **Mốc quan trọng: robot chắc chắn tới đích** (đủ qua vòng loại).

### Tuần 5: Speed run
- [ ] RUN1 ổn định → RUN2 → RUN3 ([04](04-calib-va-tinh-chinh.md) bước 9).
- [ ] Luyện trên mê cung có đoạn zig-zag (để thử chạy chéo) và chữ U (F180).
- [ ] Ghi bảng thời gian ước lượng vs thực tế.

### Tuần 6: Tổng duyệt
- [ ] Mô phỏng ngày thi: mỗi lượt đúng 3 phút tính cả chuẩn bị, theo [08](08-chien-luoc-thi.md) mục 4.
- [ ] Đã có câu trả lời của BTC → chốt chiến lược, chốt bản đồ nạp sẵn (hoặc `--none`).
- [ ] **Đóng băng firmware**: lưu lại bản chạy ổn nhất (copy thư mục / commit git) và không sửa code lớn sau đó.

## Checklist mang theo ngày thi

- [ ] Robot + **2–3 pin đã sạc đầy** + sạc cân bằng
- [ ] Laptop đã cài PlatformIO, **đã build sẵn** (không cần internet), ST-Link, USB-UART, cáp
- [ ] Băng dính (lau bánh), tua vít, mỏ hàn mini, dây dự phòng, linh kiện dự phòng
- [ ] Bản in bảng chế độ ([03](03-firmware-nap-va-van-hanh.md) mục 3) và kế hoạch lượt ([08](08-chien-luoc-thi.md) mục 3)
- [ ] Sổ ghi chép (mẫu ở [08](08-chien-luoc-thi.md) mục 7)

## Checklist trước mỗi vòng

- [ ] Đổi mê cung? → **CLEAR MAP**
- [ ] Mode SENSORS trên sa bàn thật: tường L/F/R nhận đúng, pin ≥ 7.6 V
- [ ] Bánh sạch
- [ ] Chọn sẵn chế độ cho lượt 1 trước khi tới lượt
- [ ] Rút dây UART / ST-Link
