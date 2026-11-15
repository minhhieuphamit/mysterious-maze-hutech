# CLAUDE.md

Micromouse cho cuộc thi MYSTERIOUS MAZE 2026 (HUTECH). Firmware STM32 (PlatformIO, Arduino core) + thư viện thuật toán C++ thuần dùng chung cho firmware, mô phỏng và test trên PC. Người dùng viết tiếng Việt; tài liệu viết tiếng Việt có dấu, comment trong code viết tiếng Việt **không dấu**.

## Đừng đọc (tốn token, không cần)

- `index.html` (330 KB, **tự sinh** từ `docs/`): sửa `docs/*.md` rồi tạo lại bằng lệnh bên dưới.
- `LICENSE` (văn bản GPL-3.0 chuẩn), `build/`, `.pio/` (output build, đã ignore).
- `src/preset_maze.h` (**tự sinh** bởi `scripts/maze_to_header.py`).
- `docs/` chỉ đọc khi cần sửa tài liệu. Tóm tắt nội dung từng file nằm trong `README.md`.

## Bản đồ thư mục

```
src/                 Firmware (chỉ build trên STM32)
  config.h           MỌI thông số: kích thước mê cung, cơ khí, PID/FF, cảm biến, tốc độ, chân
  main.cpp           Menu 10 chế độ (enum Mode), runSearch(), runSpeed()
  robot.*            RobotIO (MouseIO cho robot thật), runSmooth() thực thi speed run
  motion.*           Ngắt điều khiển (TIM2, 1 kHz F411 / 500 Hz F103): profile, PD, FF, steering, đường cong
  profile.h          Profile hình thang; vị trí TÍCH LŨY (extend cộng vào target cũ)
  hardware.*         encoders (TIM3/TIM4 encoder mode, set thanh ghi), motors (TIM1 PWM), sensors (IR sáng-tối), ui
  storage.*          Lưu Maze vào EEPROM giả lập (eeprom_buffer_*), có CRC
lib/MazeCore/src/    Thuật toán, KHÔNG phụ thuộc Arduino (biên dịch được trên PC)
  maze_types.h       Heading/Turn/Location; (0,0) góc dưới trái, NORTH=+y
  maze.*             1 byte/ô: bit0-3 tường NESW, bit4-7 "đã biết"; flood() BFS; Mask OPEN/CLOSED/HINT; bản đồ gợi ý
  explorer.*         MouseIO (giao diện phần cứng) + Explorer: searchToGoals/searchTo/exploreUntilSolved
  planner.*          planPath (Dijkstra ô×hướng, heap), buildFastRun (Dijkstra ô×hướng×nước trước, cost = ms)
  turns.*            Hình học cua slalom; before/after TỰ TÍNH bằng tích phân số
  smooth_path.*      Dịch chuỗi ô -> chuyển động (port từ Bulebule mmlib, GPL), walkSegments(), ước lượng thời gian
  run_defaults.h     Bán kính cua mặc định cho ô 192 mm
test/host/           test_core.cpp (thăm dò, bản đồ gợi ý, khớp preset header), test_speedrun.cpp (slalom, mô phỏng động học, --svg)
sim/main_sim.cpp     Adapter mms (stdin/stdout)
scripts/             build_host.ps1, maze_from_image.py, maze_to_header.py, site/ (tạo index.html)
mazes/               Mê cung BTC (định dạng mazefiles: o --- |)
docs/00..09          Tài liệu người dùng (danh sách trong README.md)
```

## Khái niệm cốt lõi (đọc trước khi sửa logic)

- **Ô:** 180 mm lọt lòng + tường 12 mm → **bước ô 192 mm**, `HALF_CELL` 96. Khác chuẩn quốc tế 180.
- **Hệ quy chiếu cạnh ô:** mọi chuyển động nối tiếp qua trung điểm cạnh ô. Khi thăm dò, robot đọc tường tại "điểm cảm biến" = cạnh + `SENSING_OFFSET`. `MouseIO::move(t)` = rẽ t rồi vào ô kế, dừng lại tại điểm cảm biến của ô đó.
- **Chuỗi ô (move string):** `B` (start → cạnh đầu), `F`/`L`/`R` (một ký tự cho mỗi ô đã vào), `S` (dừng ở tâm ô đích), `l/r/a` (quay tại chỗ lúc xuất phát). `smoothPath()` dịch chuỗi này sang các kiểu cua SS90/F90/F180/D45/D135/V90 và đoạn chéo.
- **Cua slalom:** độ cong biến thiên theo QUÃNG ĐƯỜNG (sin - đều - sin). Trong ngắt: `rotDelta = curveShape(s) / R × fwdDelta`. Kết thúc cua thì `fwd.shiftTarget(length)`.
- **An toàn:** speed run chỉ dùng tường ĐÃ ĐO (mask CLOSED). Bản đồ gợi ý (HINT) chỉ dùng để chọn hướng khi thăm dò.
- **Bộ nhớ tĩnh:** không dùng heap cấp phát động. Planner/flood dùng buffer static (không reentrant). F103 build với `-DMM_LOW_RAM` (tắt planner 3072 trạng thái).

## Lệnh

Trình biên dịch C++ trên máy là **zig** (máy không có g++): `python -m ziglang c++`.

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```
Build 3 chương trình vào `build/` và chạy cả 2 test. Phải ra 2 dòng `KET QUA: OK`.

```bash
python -m platformio run -e blackpill_f411ce -e bluepill_f103c8
```
Build firmware cho cả 2 board. Phải `SUCCESS` cả 2; F103 RAM ~89%, cẩn thận khi thêm buffer.

```bash
build/test_core.exe mazes/mysterious_maze_2026.txt
```

```bash
build/test_speedrun.exe --svg build/x.svg mazes/mysterious_maze_2026.txt 45 --hint mazes/mysterious_maze_2026.txt
```
Xuất hình SVG. Tham số 45 = nửa bề rộng thân robot (mm); bỏ `--hint ...` để chạy SEARCH thường.

```bash
python scripts/maze_to_header.py mazes/mysterious_maze_2026.txt
```
Tạo lại `src/preset_maze.h`; test_core kiểm tra header khớp file.

```bash
python -X utf8 scripts/site/build_site.py . index.html
```
Tạo lại trang web từ `docs/` (cần `build/btc_search.svg` và `build/btc_search_map.svg`, tạo bằng `--svg`).

## Bẫy đã gặp

- **STM32duino 3.x:** lớp UART cụ thể là `Uart`, không phải `HardwareSerial` (đã `#if` trong hardware.*). `PWM_FREQUENCY`/`PWM_RESOLUTION` là macro của core, nên không đặt tên hằng trùng.
- **printf của newlib-nano không in số thực:** in số nguyên.
- **Bash heredoc + Python trên Windows làm hỏng ký tự tiếng Việt và `\n`** trong chuỗi. Sửa file bằng công cụ Edit/Write, hoặc viết script `.py` UTF-8 rồi chạy `python -X utf8`.
- **Liên kết trong `docs/` dùng đường dẫn tương đối** (`02-phan-cung.md`). `build_site.py` đổi chúng thành `#slug`. Không đặt link Markdown bên trong khối code.
- **Đổi tốc độ hoặc bán kính cua** thì chạy `test_speedrun.exe <nửa bề rộng>` và xem "khe ho nho nhat" (âm = đâm tường).
- **Đổi `LOOP_FREQUENCY`** thì KD tính theo tick cũng phải chỉnh lại.

## Quy ước khi làm việc

- Thông số phần cứng chỉ đặt trong `src/config.h`. Thuật toán mới đặt trong `lib/MazeCore` (không include Arduino), kèm test trong `test/host/`.
- Code chưa chạy trên robot thật. Mọi số trong `config.h` là giá trị khởi điểm, calib theo `docs/04`.
- Sau khi sửa: chạy build_host.ps1 + build firmware 2 board. Sửa `docs/` thì tạo lại `index.html`.
- Commit **không** thêm dòng `Co-Authored-By`.
