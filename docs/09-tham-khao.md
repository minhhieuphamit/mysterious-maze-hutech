# Mã nguồn tham khảo và license

**Project này phát hành theo GPL-3.0** (file `LICENSE`) vì có port code từ Bulebule/mmlib (GPL-3.0). Với cuộc thi thì không ảnh hưởng gì. Chỉ khi phân phối firmware/mã nguồn cho người khác thì phải kèm mã nguồn và giữ license GPL-3.0.

| Repo | License | Dùng thế nào trong project |
|---|---|---|
| [Bulebots/mmlib](https://github.com/Bulebots/mmlib) (thư viện của [Bulebots/bulebule](https://github.com/Bulebots/bulebule)) | **GPL-3.0** | **Port code:** từ điển dịch đường đi `path.c` → `smooth_path.cpp` (đã đối chiếu với `tests/test_path.py` của họ). **Lấy ý tưởng:** đường cong có độ cong hình sin theo quãng đường (`speed.c`), các kiểu rẽ 45/90/135/180/V90, tốc độ rẽ theo lực bám |
| [ukmars/mazerunner-core](https://github.com/ukmars/mazerunner-core) | MIT | Kiến trúc firmware: profile hình thang, điều khiển PD sai số tích lũy, đọc IR sáng−tối, hệ quy chiếu cạnh ô |
| [kerikun11/micromouse-maze-library](https://github.com/kerikun11/micromouse-maze-library) | MIT | Tham khảo cấu trúc thư viện mê cung (bản hiện tại đã bỏ phần slalom) |
| [mackorone/mms](https://github.com/mackorone/mms) | MIT | Trình mô phỏng. Project có adapter `sim/` |
| [mackorone/mms-cpp](https://github.com/mackorone/mms-cpp) | MIT | Template giao thức stdin/stdout của mms |
| [micromouseonline/mazefiles](https://github.com/micromouseonline/mazefiles) | (chưa ghi license) | Mê cung thi đấu thật dạng text, dùng cho mms và test |
| [ukmars/ukmarsbot](https://github.com/ukmars/ukmarsbot) | MIT | Thiết kế phần cứng robot tham khảo |

Những phần viết mới (không có trong repo nào ở trên):
- Tự tính `before/after` của từng kiểu rẽ bằng tích phân số cho ô 192 mm.
- Planner Dijkstra theo thời gian trên trạng thái (ô, hướng, nước đi trước), kèm chọn ứng viên theo thời gian ước lượng.
- Kiểm tra khả thi, tự hạ cấp ngôn ngữ, mô phỏng động học đo khe hở tường trên 300 mê cung.
