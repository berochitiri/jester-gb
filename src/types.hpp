#pragma once

#include <cstdint>

namespace jester {

// shorter names because i'm lazy as fuck
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;

// sizes and shit
constexpr u16 SCREEN_WIDTH = 160;
constexpr u16 SCREEN_HEIGHT = 144;
constexpr u16 TERM_WIDTH = 80;
constexpr u16 TERM_HEIGHT = 36;

// cpu be zoomin at 4mhz
constexpr u32 CPU_CLOCK_HZ = 4194304;

// frame timing (dont touch this)
constexpr u32 CYCLES_PER_FRAME = 70224;
constexpr double FRAME_TIME_MS = 16.742;

// where everything lives in memory
constexpr u16 ROM_BANK_0_START = 0x0000;
constexpr u16 ROM_BANK_0_END = 0x3FFF;
constexpr u16 ROM_BANK_N_START = 0x4000;
constexpr u16 ROM_BANK_N_END = 0x7FFF;
constexpr u16 VRAM_START = 0x8000;
constexpr u16 VRAM_END = 0x9FFF;
constexpr u16 EXT_RAM_START = 0xA000;
constexpr u16 EXT_RAM_END = 0xBFFF;
constexpr u16 WRAM_START = 0xC000;
constexpr u16 WRAM_END = 0xDFFF;
constexpr u16 ECHO_RAM_START = 0xE000;
constexpr u16 ECHO_RAM_END = 0xFDFF;
constexpr u16 OAM_START = 0xFE00;
constexpr u16 OAM_END = 0xFE9F;
constexpr u16 IO_START = 0xFF00;
constexpr u16 IO_END = 0xFF7F;
constexpr u16 HRAM_START = 0xFF80;
constexpr u16 HRAM_END = 0xFFFE;
constexpr u16 IE_REGISTER = 0xFFFF;

// ppu registers (pain in the ass)
constexpr u16 LCDC = 0xFF40;
constexpr u16 STAT = 0xFF41;
constexpr u16 SCY = 0xFF42;
constexpr u16 SCX = 0xFF43;
constexpr u16 LY = 0xFF44;
constexpr u16 LYC = 0xFF45;
constexpr u16 DMA = 0xFF46;
constexpr u16 BGP = 0xFF47;
constexpr u16 OBP0 = 0xFF48;
constexpr u16 OBP1 = 0xFF49;
constexpr u16 WY = 0xFF4A;
constexpr u16 WX = 0xFF4B;

// buttons
constexpr u16 JOYP = 0xFF00;

// interrupts logic
constexpr u16 IF_REG = 0xFF0F;
constexpr u16 IE_REG = 0xFFFF;

// interrupt flags (bits)
constexpr u8 INT_VBLANK = 0x01;
constexpr u8 INT_LCD = 0x02;
constexpr u8 INT_TIMER = 0x04;
constexpr u8 INT_SERIAL = 0x08;
constexpr u8 INT_JOYPAD = 0x10;

enum class Color : u8 { White = 0, LightGray = 1, DarkGray = 2, Black = 3 };

} // namespace jester
