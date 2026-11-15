// Luu ban do me cung vao Flash (EEPROM gia lap cua STM32duino) de cac luot sau
// chay toc do ma khong can tham do lai. BTC cho phep doi phan mem giua cac luot.
#pragma once
#include "maze.h"

namespace storage {
bool saveMaze(const mm::Maze& maze);
bool loadMaze(mm::Maze& maze);  // false neu chua co / hong du lieu
void clearMaze();
}  // namespace storage
