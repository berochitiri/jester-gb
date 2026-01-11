#pragma once

#include "types.hpp"
#include <string>

namespace jester {

class Terminal {
public:
  Terminal();
  ~Terminal();

  void init();
  void cleanup();

  void clearScreen();
  void moveCursor(u16 x, u16 y);
  void hideCursor();
  void showCursor();
  void setColor(u8 r, u8 g, u8 b);
  void resetColor();
  void write(const std::string &text);
  void flush();

  static u32 pixelsToBraille(bool dots[8]);
  static std::string brailleToUTF8(u32 codepoint);
  static std::string toBraille(bool dots[8]);
  static u32 brailleCodepoint(bool dots[8]);
};

} // namespace jester
