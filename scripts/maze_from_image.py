# Nhan dang me cung tu anh (vd. Hinh 1 trong the le BTC): tuong mau DO tren nen trang, luoi 16x16.
# Dung: python scripts/maze_from_image.py anh.png mazes/ten.txt
# Can: pip install numpy pillow. O dich mac dinh: 4 o trung tam.
import sys
import numpy as np
from PIL import Image
N = 16
img = np.asarray(Image.open(sys.argv[1]).convert('RGB')).astype(int)
R, G, B = img[..., 0], img[..., 1], img[..., 2]
red = (R > 180) & (G < 90) & (B < 90)
H, W = red.shape
# Khung ngoai: hang/cot co nhieu pixel do nhat o 2 dau
cols = red.sum(axis=0); rows = red.sum(axis=1)
def edge(arr, lo, hi):
    idx = [i for i in range(lo, hi) if arr[i] > 0.8 * arr.max()]
    return (min(idx) + max(idx)) / 2 if idx else None
x0 = edge(cols, 0, 60); x1 = edge(cols, W - 60, W)
y0 = edge(rows, 0, 60); y1 = edge(rows, H - 60, H)
px = (x1 - x0) / N; py = (y1 - y0) / N
print(f"khung x {x0:.1f}..{x1:.1f}  y {y0:.1f}..{y1:.1f}  buoc o {px:.2f} x {py:.2f} px", file=sys.stderr)

def frac_h(xa, xb, y):   # tuong ngang: lay mau doc theo x, cua so +-4 px theo y
    xs = np.arange(int(xa), int(xb))
    yl, yh = max(0, int(round(y)) - 4), int(round(y)) + 5
    return red[yl:yh, xs].any(axis=0).mean()
def frac_v(x, ya, yb):
    ys = np.arange(int(ya), int(yb))
    xl, xh = max(0, int(round(x)) - 4), int(round(x)) + 5
    return red[ys, xl:xh].any(axis=1).mean()

# north[i][j]: tuong phia Bac o (i,j) (j=0 la hang duoi); east[i][j]
north = [[False]*N for _ in range(N)]; east = [[False]*N for _ in range(N)]
amb = []
for i in range(N):
    for j in range(N):
        cx0 = x0 + px * i; cx1 = cx0 + px
        ytop = y0 + py * (N - 1 - j)          # canh Bac cua o (i,j) trong anh
        f = frac_h(cx0 + 0.25*px, cx1 - 0.25*px, ytop)
        north[i][j] = f > 0.5
        if 0.15 < f < 0.85: amb.append(('N', i, j, round(f, 2)))
        xr = x0 + px * (i + 1)
        cy0 = y0 + py * (N - 1 - j); cy1 = cy0 + py
        f = frac_v(xr, cy0 + 0.25*py, cy1 - 0.25*py)
        east[i][j] = f > 0.5
        if 0.15 < f < 0.85: amb.append(('E', i, j, round(f, 2)))
print("Mau khong ro rang (can xem lai):", amb if amb else "khong co", file=sys.stderr)

goals = {(7, 7), (7, 8), (8, 7), (8, 8)}
lines = []
for j in range(N - 1, -1, -1):
    s = 'o'
    for i in range(N):
        s += ('---' if north[i][j] else '   ') + 'o'
    lines.append(s)
    s = '|'
    for i in range(N):
        mid = ' G ' if (i, j) in goals else (' S ' if (i, j) == (0, 0) else '   ')
        s += mid + ('|' if (east[i][j] or i == N - 1) else ' ')
    lines.append(s)
lines.append('o' + '---o' * N)
open(sys.argv[2], 'w', newline='\n').write('\n'.join(lines) + '\n')
print('\n'.join(lines))
