#include "storage.h"

#include <Arduino.h>
#include <EEPROM.h>  // keo thu vien EEPROM (bo dem flash) vao build

// EEPROM.h khai bao 1 bien static khong dung -> tat canh bao cho gon
static inline void touchEeprom() { (void)EEPROM; }

namespace storage {

namespace {
constexpr uint8_t kMagic0 = 'M';
constexpr uint8_t kMagic1 = 'Z';
constexpr uint8_t kVersion = 1;
// Bo cuc: [M][Z][version][size][walls x kCellCount][crc lo][crc hi]
constexpr uint32_t kHeader = 4;
static_assert(kHeader + mm::kCellCount + 2 <= E2END + 1, "Ban do khong vua vung EEPROM gia lap");
}  // namespace

bool saveMaze(const mm::Maze& maze) {
  eeprom_buffer_fill();
  eeprom_buffered_write_byte(0, kMagic0);
  eeprom_buffered_write_byte(1, kMagic1);
  eeprom_buffered_write_byte(2, kVersion);
  eeprom_buffered_write_byte(3, mm::kMazeSize);
  for (uint16_t i = 0; i < mm::kCellCount; i++) eeprom_buffered_write_byte(kHeader + i, maze.rawCell(i));
  const uint16_t crc = maze.checksum();
  eeprom_buffered_write_byte(kHeader + mm::kCellCount, uint8_t(crc & 0xFF));
  eeprom_buffered_write_byte(kHeader + mm::kCellCount + 1, uint8_t(crc >> 8));
  eeprom_buffer_flush();  // xoa + ghi 1 lan (vai tram ms, robot phai dung yen)
  return true;
}

bool loadMaze(mm::Maze& maze) {
  eeprom_buffer_fill();
  if (eeprom_buffered_read_byte(0) != kMagic0 || eeprom_buffered_read_byte(1) != kMagic1 ||
      eeprom_buffered_read_byte(2) != kVersion || eeprom_buffered_read_byte(3) != mm::kMazeSize)
    return false;
  mm::Maze tmp;
  for (uint16_t i = 0; i < mm::kCellCount; i++) tmp.setRawCell(i, eeprom_buffered_read_byte(kHeader + i));
  const uint16_t crc = uint16_t(eeprom_buffered_read_byte(kHeader + mm::kCellCount)) |
                       uint16_t(eeprom_buffered_read_byte(kHeader + mm::kCellCount + 1)) << 8;
  if (crc != tmp.checksum()) return false;
  for (uint16_t i = 0; i < mm::kCellCount; i++) {
    maze.setRawCell(i, tmp.rawCell(i));
    // O co du 4 tuong da biet coi nhu da di qua
    if ((tmp.rawCell(i) & 0xF0) == 0xF0) maze.markVisited(mm::Location::fromIndex(i));
  }
  return true;
}

void clearMaze() {
  eeprom_buffer_fill();
  eeprom_buffered_write_byte(0, 0xFF);
  eeprom_buffer_flush();
}

}  // namespace storage
