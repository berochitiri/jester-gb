#pragma once

#include "types.hpp"
#include <array>

namespace jester {

class PPU {
public:
  PPU();

  void reset();
  void step(u32 cycles);

  u8 readVRAM(u16 addr) const;
  void writeVRAM(u16 addr, u8 val);
  u8 readOAM(u16 addr) const;
  void writeOAM(u16 addr, u8 val);
  u8 readRegister(u16 addr) const;
  void writeRegister(u16 addr, u8 val);

  bool isFrameReady() const { return frameReady; }
  void clearFrameReady() { frameReady = false; }
  const std::array<u8, 160 * 144> &getFrameBuffer() const {
    return frameBuffer;
  }

  bool hasVBlankInterrupt() const { return vblankInterrupt; }
  void clearVBlankInterrupt() { vblankInterrupt = false; }
  bool hasStatInterrupt() const { return statInterrupt; }
  void clearStatInterrupt() { statInterrupt = false; }

private:
  std::array<u8, 0x2000> vram;
  std::array<u8, 160> oam;
  std::array<u8, 160 * 144> frameBuffer;

  u8 lcdc = 0x91, stat = 0, scy = 0, scx = 0;
  u8 ly = 0, lyc = 0, bgp = 0xFC, obp0 = 0, obp1 = 0;
  u8 wy = 0, wx = 0;
  u8 windowLine = 0;
  u8 mode = 0;

  u32 modeClock = 0;
  bool frameReady = false;
  bool vblankInterrupt = false;
  bool statInterrupt = false;

  static constexpr u8 MODE_HBLANK = 0;
  static constexpr u8 MODE_VBLANK = 1;
  static constexpr u8 MODE_OAM = 2;
  static constexpr u8 MODE_VRAM =
      3; // mode_transfer is a dumb name so i changed it to vram

  static constexpr u8 LINES_VISIBLE = 144;
  static constexpr u8 LINES_TOTAL = 154;

  static constexpr u32 CYCLES_OAM = 80;
  static constexpr u32 CYCLES_VRAM = 172;
  static constexpr u32 CYCLES_HBLANK = 204;
  static constexpr u32 CYCLES_LINE = 456;

  void setMode(u8 newMode);
  void checkLYC();
  void renderScanline();
  void renderBackground(u8 scanline);
  void renderWindow(u8 scanline);
  void renderSprites(u8 scanline);
  u8 getColorFromPalette(u8 colorNum, u8 palette) const;
};

} // namespace jester
