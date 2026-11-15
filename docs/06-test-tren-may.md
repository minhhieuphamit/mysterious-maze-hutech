# Hướng dẫn test trên máy tính (chưa cần robot)

Có 3 cách, từ nhanh tới trực quan:

| Cách | Xem được gì | Cần cài |
|---|---|---|
| A. Test tự động | Thuật toán đúng hay sai trên 300 mê cung, thời gian ước lượng | Python |
| B. Xuất hình SVG | **Hình mê cung + đường thăm dò + quỹ đạo speed run có cua cong và chéo, có hoạt ảnh** | Python + trình duyệt |
| C. Trình mô phỏng mms | Xem chuột thăm dò từng ô theo thời gian thực | Python + mms |

Mọi lệnh dưới đây chạy trong thư mục `D:\Competition\mysterious-maze`, bằng PowerShell hoặc Terminal của VS Code.

## 0. Cài một lần

```bash
python -m pip install ziglang platformio
```

`ziglang` là trình biên dịch C++ để chạy test trên PC. `platformio` dùng để build/nạp firmware STM32.

## A. Chạy test tự động

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```

Script build 3 chương trình vào thư mục `build/` và chạy 2 bộ test. Kết quả cần thấy:

```
[preset_maze.h] mysterious_maze_2026.txt: 0 o khac voi file
[random] 300 me cung ngau nhien 16x16
  thanh cong 300/300 | ban do toi uu 300/300 | duong speed run toi uu 300/300
[ban do goi y] 100 me cung ngau nhien (trung binh)
  khong nap map          thanh cong 100/100 |  85 o toi dich | 168 o tong | ...
  nap map DUNG           thanh cong 100/100 |  38 o toi dich |  78 o tong | ...
  ...
KET QUA: OK
...
  thoi gian TB (uoc luong, s): chi SS90=8.01 | SAFE (F90/F180)=7.43 | CHEO=5.75 | ...
  khe ho nho nhat voi tuong/cot: 12.3 mm (V90)
KET QUA: OK
```

- `preset_maze.h ... 0 o khac`: bản đồ nạp vào firmware khớp file nguồn ([07](07-nap-ban-do-san.md)).
- `thanh cong 300/300`: robot tìm được đích trên mọi mê cung.
- `ban do goi y`: so sánh SEARCH với SEARCH+MAP khi bản đồ nạp đúng, sai vài tường, hoặc sai hẳn.
- `thoi gian TB`: thời gian speed run ước lượng của từng kiểu chạy.
- `khe ho nho nhat`: khoảng hở nhỏ nhất giữa thân robot và tường khi chạy cua/chéo. **Âm nghĩa là đâm tường.**

Kiểm tra với bề rộng robot thật. Tham số là **nửa bề rộng thân robot** (mm), ví dụ robot rộng 96 mm thì truyền 48:

```bash
build/test_speedrun.exe 48
```

## B. Xem hình mê cung và đường chạy (khuyên dùng)

```bash
build/test_speedrun.exe --svg build/maze.svg 1
```

Mở `build/maze.svg` bằng Chrome/Edge (kéo thả file vào trình duyệt). Đường thăm dò được vẽ dần trước, sau đó tới speed run.

| Màu | Ý nghĩa |
|---|---|
| Xanh dương mờ | Đường chuột đi khi thăm dò |
| Vàng | Speed run: đoạn thẳng |
| Hồng | Speed run: đoạn **chéo** |
| Xanh lá | Speed run: **đường cong** (F90, F180, 45°, 135°, V90) |
| Ô xám | Ô đã đi qua; ô nâu = vùng đích |

Đổi số `1` thành số khác (2, 3, 7, 42…) để xem mê cung ngẫu nhiên khác. Muốn thêm bán kính thân robot để tính khe hở thì truyền thêm tham số cuối:

```bash
build/test_speedrun.exe --svg build/maze.svg 7 48
```

### Mê cung mẫu trong thể lệ BTC (có sẵn)

File [`mazes/mysterious_maze_2026.txt`](../mazes/mysterious_maze_2026.txt) được nhận dạng tự động từ Hình 1 trong thể lệ: trích ảnh gốc trong PDF, lấy mẫu pixel đỏ trên từng cạnh ô, không có cạnh nào mập mờ.

```bash
build/test_speedrun.exe --svg build/btc.svg mazes/mysterious_maze_2026.txt 45
```

```bash
build/test_core.exe mazes/mysterious_maze_2026.txt
```

Kết quả hiện tại: tới đích sau 73 ô, thăm dò tổng 246 ô thì tìm ra đường tối ưu (~150 s ở 0.35 m/s). Speed run chạy chéo ước lượng 7.3 s.

**Mô phỏng chế độ SEARCH+MAP** (dùng bản đồ nạp sẵn làm gợi ý): thêm `--hint <file>`. Chi tiết ở [07](07-nap-ban-do-san.md).

```bash
build/test_speedrun.exe --svg build/btc_map.svg mazes/mysterious_maze_2026.txt 45 --hint mazes/mysterious_maze_2026.txt
```

### Dùng mê cung thi đấu khác

1. Tải bộ mê cung: https://github.com/micromouseonline/mazefiles (nút **Code → Download ZIP**), giải nén, ví dụ vào `D:\Competition\mazefiles`.
2. Chạy với một file 16×16 trong thư mục `classic`:

```bash
build/test_speedrun.exe --svg build/apec2010.svg D:/Competition/mazefiles/classic/apec2010.txt
```

3. **Tự vẽ mê cung của BTC** (khi biết sơ đồ, ví dụ hình mẫu trong thể lệ): tạo file `.txt` theo đúng định dạng đó. `o` là cột, `---` là tường ngang, `|` là tường dọc, `G` là ô đích, `S` là ô xuất phát. Ví dụ 4×4:

```
o---o---o---o---o
| G |           |
o   o   o   o---o
|       |       |
o---o---o---o   o
|               |
o   o---o---o   o
| S |           |
o---o---o---o---o
```

Kiểm tra cả bộ test thăm dò trên file đó:

```bash
build/test_core.exe D:/Competition/mazefiles/classic/apec2010.txt
```

## C. Trình mô phỏng mms (xem thăm dò theo thời gian thực)

1. Tải mms tại https://github.com/mackorone/mms. Bản build sẵn nằm trong mục **Releases**; nếu không có bản cho Windows thì phải build từ mã nguồn bằng Qt theo README của họ.
2. Mở mms. Ở khung **Mouse**, bấm **+** và điền:
   - **Name:** `mysterious-maze`
   - **Directory:** `D:\Competition\mysterious-maze`
   - **Build Command:** `powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1` (hoặc để trống nếu đã build)
   - **Run Command:** `build\mouse_sim.exe`
3. Ở khung **Maze**, chọn một file 16×16 trong thư mục `mazefiles/classic`.
4. Bấm **Build**, rồi **Run**.

Bạn sẽ thấy:
- Lượt 1: chuột dò từ góc dưới trái vào đích, số trên mỗi ô là giá trị Flood Fill, tường được vẽ dần khi phát hiện.
- Chuột tiếp tục dò thêm rồi về Start.
- Lượt 2: đường speed run tô vàng, chuột chạy theo.
- Khung log bên dưới in số bước và chuỗi lệnh.

> mms chỉ di chuyển theo ô (không vẽ được đường cong), nên trong mms speed run hiển thị theo ô vuông. Muốn xem cua cong và chạy chéo thì dùng cách B.

## Khi sửa code / thông số

| Sửa gì | Kiểm tra bằng |
|---|---|
| Thuật toán thăm dò (`explorer.cpp`, `maze.cpp`) | A (`test_core`) + C (mms) |
| Bán kính cua (`run_defaults.h`), tốc độ (`RUN_PARAMS`) | A (`test_speedrun <nửa bề rộng>`) + B |
| Firmware (`src/`) | `python -m platformio run` phải ra `SUCCESS` cho cả 2 board |

Lưu ý: test trên máy giả định cảm biến hoàn hảo và robot bám đúng quỹ đạo. Trên robot thật vẫn phải calib theo [04-calib-va-tinh-chinh.md](04-calib-va-tinh-chinh.md).
