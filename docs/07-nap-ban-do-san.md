# Nạp bản đồ có sẵn vào robot (chế độ SEARCH+MAP)

## Hai chế độ thăm dò

| Chế độ | Khi nào dùng | Robot làm gì |
|---|---|---|
| **SEARCH** | Không biết trước mê cung | Flood Fill bình thường ([05-thuat-toan.md](05-thuat-toan.md)) |
| **SEARCH+MAP** | Có bản đồ dự đoán (vd. Hình 1 trong thể lệ, hoặc BTC công bố trước) | Dùng bản đồ nạp sẵn để **chọn hướng đi**, nhưng **vẫn đọc cảm biến ở mọi ô** |

Bản đồ nạp sẵn chỉ là **gợi ý**, không được tin tuyệt đối:
- Robot so mỗi tường đo được với gợi ý. Chỗ nào khác thì dùng số đo thật và tính đường lại.
- Lệch quá `PRESET_MISMATCH_TOLERANCE` tường (mặc định 4), hoặc gợi ý chặn hết đường, thì robot **bỏ gợi ý** và thăm dò như SEARCH thường.
- **Speed run chỉ dùng tường robot đã tự đo.** Map nạp sai cũng không làm robot chạy nhanh vào tường.

## Kết quả mô phỏng

Mê cung mẫu của BTC (`mazes/mysterious_maze_2026.txt`):

| Trường hợp | Tới đích | Tổng thăm dò | Speed run |
|---|---|---|---|
| SEARCH (không nạp map) | 73 ô | 246 ô (~150 s) | tối ưu |
| SEARCH+MAP, map **đúng** | **57 ô** | **120 ô (~75 s)** | tối ưu |
| SEARCH+MAP, map **sai 6 tường** | 57 ô | 120 ô | tối ưu (phát hiện 2 tường lệch trên đường đi) |
| SEARCH+MAP, map của **mê cung khác** | 73 ô | 246 ô | tối ưu (lệch 3 tường → **tự bỏ map**) |

Trên 100 mê cung ngẫu nhiên:

| Trường hợp | Tới đích | Tổng thăm dò | Speed run tối ưu |
|---|---|---|---|
| Không nạp map | 85 ô | 168 ô | 100/100 |
| Map đúng | **38 ô** | **78 ô** | 100/100 |
| Map sai 6 tường | 54 ô | 103 ô | 92/100 ⚠️ |
| Map mê cung khác | 86 ô | 166 ô | 100/100 (bỏ map 100/100) |

⚠️ Khi map **gần đúng nhưng sai vài chỗ**, robot tin những tường nó chưa đi qua. Nếu map vẽ thừa một tường mà ngoài thực tế là lối đi tắt, robot sẽ không biết đường tắt đó. Vì vậy SEARCH+MAP **chỉ có lợi khi map gần như chắc chắn đúng**. Nếu nghi ngờ, dùng SEARCH thường.

## Xem bằng hình trên máy tính

Thêm `--hint <file bản đồ gợi ý>` vào lệnh xuất SVG để mô phỏng chế độ SEARCH+MAP:

```bash
build/test_speedrun.exe --svg build/btc_search_map.svg mazes/mysterious_maze_2026.txt 45 --hint mazes/mysterious_maze_2026.txt
```

So sánh với SEARCH thường (không có `--hint`):

```bash
build/test_speedrun.exe --svg build/btc_search.svg mazes/mysterious_maze_2026.txt 45
```

Muốn thử map nạp **sai**, sửa vài tường trong một bản sao file `.txt` rồi truyền bản sao đó vào `--hint`. Tiêu đề hình sẽ ghi số tường lệch và robot có bỏ map hay không.

## Cách nạp bản đồ

### Bước 1: Có file mê cung `.txt`

**Từ ảnh** (tường màu đỏ trên nền trắng, lưới 16×16 như hình trong thể lệ). Cắt ảnh sơ đồ (Snipping Tool) sát khung đỏ ngoài cùng, rồi:

```bash
python scripts/maze_from_image.py anh_so_do.png mazes/ban_do_moi.txt
```

Script in ra mê cung dạng chữ và báo những cạnh nhận dạng không rõ (nếu có). **Luôn kiểm tra lại bằng hình:**

```bash
build/test_speedrun.exe --svg build/kiem_tra.svg mazes/ban_do_moi.txt
```

Mở `build/kiem_tra.svg` và so với ảnh gốc.

**Hoặc tự vẽ** bằng Notepad theo định dạng `o --- |` (xem [06-test-tren-may.md](06-test-tren-may.md)).

### Bước 2: Chuyển thành header và nạp vào robot

```bash
python scripts/maze_to_header.py mazes/ban_do_moi.txt
```

```bash
powershell -ExecutionPolicy Bypass -File scripts/build_host.ps1
```

Bước test này kiểm tra header khớp file: dòng `[preset_maze.h] ... 0 o khac voi file`.

```bash
python -m platformio run -t upload
```

Hiện tại `src/preset_maze.h` đã chứa sẵn mê cung mẫu của BTC.

Bỏ bản đồ nạp sẵn:

```bash
python scripts/maze_to_header.py --none
```

### Bước 3: Chạy

Bấm MODE tới chế độ **2 (SEARCH+MAP)**, bấm GO, vẫy tay để xuất phát. Sau lượt chạy, UART in số tường lệch so với map và cho biết robot có bỏ map hay không. Các lượt sau dùng RUN1/RUN2/RUN3 như bình thường.

## ⚠️ Về luật

Thể lệ ghi mê cung *"được thiết kế, lắp đặt và đảm bảo tính bí mật"* và robot phải *"tự động định vị, quét bản đồ và tìm đường đi tối ưu"*. Hình 1 chỉ là sơ đồ **dự kiến**.

Nạp sẵn bản đồ có thể bị BTC coi là **trái tinh thần cuộc thi**, kể cả khi robot vẫn tự đo tường. **Hãy hỏi BTC trước** xem có được nạp bản đồ dự kiến không. Nếu không được, dùng SEARCH: thuật toán vẫn tìm ra đường tối ưu, chỉ mất thêm thời gian thăm dò ở lượt đầu.
