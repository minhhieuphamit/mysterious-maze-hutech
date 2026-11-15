# MYSTERIOUS MAZE 2026: Micromouse (STM32 + IR + DC encoder)

Project dự thi **MYSTERIOUS MAZE 2026** (Khoa Cơ khí - Tự động hóa, Trường Kỹ thuật HUTECH): robot Micromouse tự dò mê cung 16×16 (ô 180 mm lọt lòng, tường 12 mm) và chạy về 4 ô đích trung tâm trong thời gian ngắn nhất.

**Đã có:**
- Thăm dò **Flood Fill** rẽ cong không dừng. Robot tự biết khi nào bản đồ đủ để chạy tối ưu. Lưu bản đồ vào Flash.
- Speed run **slalom + chạy chéo** (F90, F180, 45°/135° vào/ra chéo, V90). Hình học cua tự tính cho ô 192 mm, planner chọn đường **nhanh nhất theo thời gian**.
- Chế độ **SEARCH+MAP**: dùng bản đồ dự kiến (đã số hóa từ hình trong thể lệ) làm gợi ý; sai thì tự bỏ.
- Bộ **test trên máy tính** và **xuất hình SVG** xem đường chạy, không cần robot.

**Kết quả mô phỏng trên mê cung mẫu của BTC:**

| | |
|---|---|
| Tới đích | 73 ô (~45 s) |
| Thăm dò đủ để chắc tối ưu | 246 ô (~150 s); 120 ô nếu dùng SEARCH+MAP |
| Speed run ước lượng | RUN1 15.8 s, RUN2 10.4 s, **RUN3 7.0 s** |
| Va chạm | Không có (thân robot rộng ≤ 100 mm) |

> ⚠️ Code đã được kiểm chứng bằng **biên dịch + mô phỏng**, chưa chạy trên robot thật của đội. Phải lắp ráp và calib theo tài liệu.

## Tài liệu (đọc theo thứ tự)

| # | Tài liệu | Nội dung |
|---|---|---|
| 00 | [Lộ trình dự án](docs/00-lo-trinh-du-an.md) | Phân vai, lộ trình 6 tuần, checklist ngày thi |
| 01 | [Tổng hợp thể lệ](docs/01-tong-hop-the-le.md) | Luật, sa bàn, vòng thi, **câu hỏi cần hỏi BTC** |
| 02 | [Phần cứng](docs/02-phan-cung.md) | BOM, chọn động cơ, mạch từng khối, sơ đồ chân, cơ khí, lắp ráp, kiểm tra |
| 03 | [Firmware: nạp và vận hành](docs/03-firmware-nap-va-van-hanh.md) | Cài đặt, build, nạp, **các chế độ, nút bấm, âm báo**, đọc log |
| 04 | [Calib và tinh chỉnh](docs/04-calib-va-tinh-chinh.md) | 10 bước calib trên robot thật, **tham chiếu mọi thông số `config.h`** |
| 05 | [Thuật toán](docs/05-thuat-toan.md) | Flood Fill, bản đồ gợi ý, slalom/chéo, planner theo thời gian, điều khiển |
| 06 | [Test trên máy tính](docs/06-test-tren-may.md) | Test tự động, **xuất hình SVG**, trình mô phỏng mms |
| 07 | [Nạp bản đồ có sẵn](docs/07-nap-ban-do-san.md) | Ảnh → file mê cung → nạp robot, chế độ SEARCH+MAP |
| 08 | [Chiến lược thi](docs/08-chien-luoc-thi.md) | Kế hoạch từng vòng/lượt, quy trình 3 phút, quản lý rủi ro |
| 09 | [Tham khảo và license](docs/09-tham-khao.md) | Repo GitHub đã dùng, GPL-3.0 |

## Bắt đầu nhanh

```bash
python -m pip install platformio ziglang numpy pillow
```

Test thuật toán trên máy (phải ra `KET QUA: OK`):

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```

Xem hình robot chạy trên mê cung BTC (mở file SVG bằng Chrome/Edge):

```bash
build/test_speedrun.exe --svg build/btc.svg mazes/mysterious_maze_2026.txt 45
```

Build và nạp firmware (BlackPill F411 qua ST-Link):

```bash
python -m platformio run -t upload
```

## Trang web sổ tay (GitHub Pages)

`index.html` ở thư mục gốc là toàn bộ tài liệu gộp thành một trang web (có menu, chế độ sáng/tối, đọc được trên điện thoại).

Bật trang: trên GitHub vào **Settings → Pages → Build and deployment → Deploy from a branch**, chọn nhánh `main` và thư mục `/ (root)`. Sau 1–2 phút trang có ở `https://<tên-tài-khoản>.github.io/<tên-repo>/`. File `.nojekyll` giữ GitHub phục vụ nguyên trạng, không xử lý qua Jekyll.

Sửa tài liệu trong `docs/` thì tạo lại trang (cần chạy `scripts/build_host.ps1` trước để có hình mê cung trong `build/`):

```bash
python -X utf8 scripts/site/build_site.py . index.html
```

## Cấu trúc thư mục

```
docs/           Tài liệu (bảng trên)
src/            Firmware STM32 (Arduino core qua PlatformIO). MỌI thông số: src/config.h
lib/MazeCore/   Thuật toán C++ thuần: dùng chung cho firmware, mô phỏng, test
test/host/      test_core (thăm dò, bản đồ gợi ý) + test_speedrun (slalom, mô phỏng động học, SVG)
sim/            Adapter cho trình mô phỏng mms
mazes/          Mê cung mẫu của BTC (số hóa từ Hình 1 trong thể lệ)
scripts/        build_host.ps1, maze_from_image.py (ảnh → mê cung), maze_to_header.py (mê cung → firmware)
```

## Phần cứng tóm tắt

STM32F411 BlackPill · 2 × N20 6V 1:20 có encoder · TB6612FNG · 4 cặp IR 940 nm (2 nhìn trước, 2 nghiêng 45°) · LiPo 2S · thân ≤ 100 mm rộng, ≤ 120 g. Chi tiết: [docs/02](docs/02-phan-cung.md).

## Chế độ robot

| # | Chế độ | Dùng khi |
|---|---|---|
| 1 | SEARCH | Lượt đầu ở mê cung mới |
| 2 | SEARCH+MAP | Có bản đồ dự kiến và BTC cho phép |
| 3 / 4 / 5 | RUN1 / RUN2 / RUN3 | Speed run 1.0 / 1.6 / 2.2 m/s (chỉ cua nhỏ / cua lớn / chạy chéo) |
| 6 | SENSORS | Calib, kiểm tra trước lượt |
| 7 / 8 | STRAIGHT4 / SPIN 720 | Calib bánh |
| 9 / 10 | PRINT MAP / CLEAR MAP | Xem / xóa bản đồ đã lưu |

MODE chọn chế độ (LED nháy N lần), GO bắt đầu, vẫy tay trước cảm biến trước để xuất phát, GO lúc đang chạy để dừng khẩn. Chi tiết: [docs/03](docs/03-firmware-nap-va-van-hanh.md).

## License

**GPL-3.0** (file `LICENSE`): có port code dịch đường đi từ Bulebots/mmlib. Xem [docs/09](docs/09-tham-khao.md).
