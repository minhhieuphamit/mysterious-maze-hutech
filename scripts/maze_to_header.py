"""Chuyen file me cung (dinh dang mazefiles: o --- |) thanh src/preset_maze.h de nap vao robot.

Dung:
    python scripts/maze_to_header.py mazes/mysterious_maze_2026.txt
    python scripts/maze_to_header.py --none          (bo ban do nap san)

Sau do build + nap firmware: python -m platformio run -t upload
Robot dung ban do nay o che do "SEARCH+MAP" (chi la GOI Y, robot van do tuong that).
"""
import os
import sys

N = 16
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src", "preset_maze.h")


def parse(path):
    lines = [l.rstrip("\r\n") for l in open(path, encoding="utf-8")]
    lines = [l for l in lines if l.strip()]
    size = len(lines) // 2
    if size != N or len(lines) != 2 * N + 1:
        sys.exit(f"File {path}: can me cung {N}x{N} ({2 * N + 1} dong), doc duoc {len(lines)} dong")
    walls = [0] * (N * N)  # bit0 N, bit1 E, bit2 S, bit3 W ; index = y*N + x

    def row(k):  # dong k tinh tu DUOI len (0 = dong cuoi file)
        return lines[len(lines) - 1 - k]

    for y in range(N):
        top, mid, bot = row(2 * y + 2), row(2 * y + 1), row(2 * y)
        for x in range(N):
            b = 0
            if len(top) > 4 * x + 2 and top[4 * x + 2] == "-":
                b |= 1
            if len(mid) > 4 * x + 4 and mid[4 * x + 4] == "|":
                b |= 2
            if len(bot) > 4 * x + 2 and bot[4 * x + 2] == "-":
                b |= 4
            if len(mid) > 4 * x and mid[4 * x] == "|":
                b |= 8
            walls[y * N + x] = b
    return walls


def write(walls, name):
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("// TU DONG TAO boi scripts/maze_to_header.py - khong sua tay.\n")
        f.write(f"// Nguon: {name}\n")
        f.write("// bit0..3 = tuong N,E,S,W ; chi so = y*16 + x (o (0,0) = goc duoi trai)\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        if walls is None:
            f.write("constexpr bool PRESET_MAZE_AVAILABLE = false;\n")
            f.write('constexpr const char* PRESET_MAZE_NAME = "";\n')
            f.write("constexpr uint8_t PRESET_MAZE_WALLS[256] = {};\n")
            return
        f.write("constexpr bool PRESET_MAZE_AVAILABLE = true;\n")
        f.write(f'constexpr const char* PRESET_MAZE_NAME = "{os.path.basename(name)}";\n')
        f.write("constexpr uint8_t PRESET_MAZE_WALLS[256] = {\n")
        for y in range(N):  # chi so tang dan: y=0 (hang duoi) truoc
            f.write("    " + ", ".join(f"{walls[y * N + x]:2d}" for x in range(N)) + f",  // y={y}\n")
        f.write("};\n")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if sys.argv[1] == "--none":
        write(None, "")
        print(f"Da xoa ban do nap san -> {OUT}")
    else:
        w = parse(sys.argv[1])
        write(w, sys.argv[1])
        print(f"Da ghi {OUT} tu {sys.argv[1]}")
