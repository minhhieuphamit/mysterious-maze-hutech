# Tổng hợp thể lệ MYSTERIOUS MAZE 2026

Nguồn: `PL2. MYSTERIOUS MAZE 2026.pdf` (Khoa Cơ khí - Tự động hóa, Trường Kỹ thuật HUTECH). Khẩu hiệu: *"Think Smart – Move Fast – Conquer the Maze"*.

## 1. Tóm tắt

| Mục | Nội dung |
|---|---|
| Loại cuộc thi | Micromouse: robot tự hành tự tìm đường trong mê cung |
| Đối tượng | Sinh viên HUTECH, đội **tối đa 3 người**, có nhóm trưởng |
| Đăng ký | Trên website Trường Kỹ thuật HUTECH (tên nhóm, thành viên, nhóm trưởng) |
| Số robot | **1 robot / đội** suốt cuộc thi |
| Thành tích | **Thời gian (mm:ss:ms)** từ lúc rời vạch Start tới khi **chạm vùng Goal** |
| Workshop | BTC có thể tổ chức hướng dẫn Flood Fill, Wall Follower, chọn phần cứng, chống nhiễu cảm biến |

## 2. Yêu cầu robot

| Quy định | Ảnh hưởng tới thiết kế |
|---|---|
| Dài × rộng **≤ 15 × 15 cm** | Project khuyên thân rộng ≤ 90–100 mm để chạy chéo ([02](02-phan-cung.md)) |
| **Hoàn toàn tự động**; cấm điều khiển từ xa, **Wi-Fi, Bluetooth, RF** | STM32 không có radio. UART debug chỉ cắm dây khi ở ngoài sa bàn |
| Được dùng cảm biến gắn trên robot: IR, siêu âm, ToF, quang, IMU | Project dùng 4 cặp IR + encoder (có thể thêm IMU) |
| Cấm định vị ngoài: GPS, camera treo, thiết bị định vị độc lập | — |
| Mê cung **bí mật** tới lúc thi | Robot phải tự dò. Hình 1 chỉ là "dự kiến" (xem [07](07-nap-ban-do-san.md) về nạp bản đồ) |

## 3. Sa bàn

| Thông số | Giá trị | Ghi chú cho thiết kế |
|---|---|---|
| Kích thước | 16 × 16 ô (dự kiến) | Đổi được bằng `-DMAZE_SIZE=N` |
| Ô | **18 × 18 cm lọt lòng** | ⚠️ Khác chuẩn quốc tế. **Bước ô = 180 + 12 = 192 mm** |
| Tường | Cao 10 ± 0.5 cm, dày 1.2 ± 0.2 cm, **trắng** | Phản xạ IR tốt |
| Nền | Đen / xám đậm | |
| Start | Ô (0,0), góc dưới trái | Hướng Bắc; ô Start có tường phía Đông (theo hình) |
| Goal | **4 ô trung tâm** (7,7) (7,8) (8,7) (8,8) | Chạm ô nào cũng dừng giờ. Theo hình: lối vào từ phía Đông |
| Đường đi | Nhiều đường hợp lệ, phải tìm **đường tối ưu** | Mê cung có vòng lặp: bám tường không đủ |
| Sàn | Ván ép / nhựa sơn mịn, ma sát vừa | Bánh cao su mềm, lau sạch |
| Sai số ghép | ≤ 0.5 mm | |
| Chiếu sáng | Đều, tránh bóng đổ | Vẫn khử ánh sáng nền (đọc sáng − tối) |

Bản đồ Hình 1 đã được số hóa: [`mazes/mysterious_maze_2026.txt`](../mazes/mysterious_maze_2026.txt).

## 4. Luật thi đấu

**Chung:**
- Tuân thủ nội quy, hướng dẫn kỹ thuật, thời gian thi của BTC; có mặt đúng giờ **Technical Check**.
- BTC có quyền từ chối/hủy kết quả robot không đạt yêu cầu, gây mất an toàn, vi phạm.
- Lượt **không hợp lệ** nếu robot: va chạm **kẹt cứng** vào tường, mất kiểm soát, **đứng yên > 30 giây**, không tới đích trong thời gian quy định.
- Giữa các lượt **được chỉnh phần mềm / tham số**, **không được thay phần cứng chính** (cảm biến, động cơ, mạch điều khiển).

| Vòng | Số lượt | Giới hạn | Điều kiện |
|---|---|---|---|
| Vòng loại | 3 | **3 phút/lượt, tính cả chuẩn bị và chỉnh tham số** | ≥ 1 lần chạm Goal là qua vòng |
| Vòng tuyển chọn | 3 | < 3 phút | **8 đội** thời gian ngắn nhất vào chung kết |
| Chung kết | **2** | | Xếp hạng theo thời gian |

## 5. Những điều thể lệ KHÔNG nói rõ (cần hỏi BTC)

1. Sau khi chạm Goal robot có được chạy tiếp (dò thêm, về Start) không?
2. Có được giữ bản đồ trong bộ nhớ giữa các lượt/vòng không?
3. Mê cung có giữ nguyên giữa các vòng không?
4. Có được nạp sẵn bản đồ dự kiến không?
5. Xuất phát bằng vẫy tay trước cảm biến có được không?
6. Thay pin cùng loại có được không?

Thể lệ **không cấm** quét toàn bộ mê cung; giới hạn thật là 3 phút/lượt. Chiến lược chi tiết theo từng câu trả lời: [08-chien-luoc-thi.md](08-chien-luoc-thi.md).
