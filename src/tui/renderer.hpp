#pragma once

#include "tui/terminal.hpp"
#include "types.hpp"
#include <array>
#include <string>

namespace jester {

class Renderer {
public:
  Renderer();

  void init(Terminal *term);
  void setPalette(u8 pal);
  void render(const std::array<u8, 160 * 144> &frameBuffer);
  void drawBorder();
  void renderDebug(u16 pc, u8 a, u8 f, u16 sp, double fps, u64 cycles);

private:
  Terminal *terminal = nullptr;
  u8 colorPalette = 0;

  static constexpr u16 BORDER_X = 1;
  static constexpr u16 BORDER_Y = 1; // screen positioning bullshit

  struct RGB {
    u8 r, g, b;
  };
  static constexpr RGB PALETTES[5][4] = {
      {{255, 255, 255}, {170, 170, 170}, {85, 85, 85}, {0, 0, 0}},
      {{155, 188, 15}, {139, 172, 15}, {48, 98, 48}, {15, 56, 15}},
      {{255, 191, 0}, {223, 159, 0}, {159, 95, 0}, {63, 31, 0}},
      {{155, 188, 255}, {100, 140, 200}, {50, 90, 150}, {20, 40, 80}},
      {{255, 155, 200}, {220, 100, 160}, {160, 60, 120}, {80, 20, 60}}};

  std::array<u32, 80 * 36> brailleBuffer;
  std::string getColorCode(u8 gbColor);
  std::string renderBlock(const std::array<u8, 160 * 144> &fb, u16 bx, u16 by);
};

} // namespace jester
