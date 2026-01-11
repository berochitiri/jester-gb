#include "tui/terminal.hpp"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

namespace jester {

Terminal::Terminal() = default;
Terminal::~Terminal() { cleanup(); }

void Terminal::init() {
#ifdef _WIN32
  // Set console to UTF-8 mode for proper Braille/Unicode rendering
  SetConsoleOutputCP(65001);
  // Enable virtual terminal processing for ANSI escape codes
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
  printf("\033[?25l");
  printf("\033[2J");
  fflush(stdout);
}

void Terminal::cleanup() {
  printf("\033[?25h");
  printf("\033[0m");
  printf("\033[2J\033[H");
  fflush(stdout);
}

void Terminal::clearScreen() {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void Terminal::moveCursor(u16 x, u16 y) { printf("\033[%d;%dH", y, x); }
void Terminal::hideCursor() { printf("\033[?25l"); }
void Terminal::showCursor() { printf("\033[?25h"); }
void Terminal::setColor(u8 r, u8 g, u8 b) {
  printf("\033[38;2;%d;%d;%dm", r, g, b);
}
void Terminal::resetColor() { printf("\033[0m"); }
void Terminal::write(const std::string &text) { printf("%s", text.c_str()); }
void Terminal::flush() { fflush(stdout); }

u32 Terminal::pixelsToBraille(bool dots[8]) { return brailleCodepoint(dots); }

u32 Terminal::brailleCodepoint(bool dots[8]) {
  u32 cp = 0x2800; // braille base offset mfs
  if (dots[0])
    cp |= 0x01;
  if (dots[1])
    cp |= 0x02;
  if (dots[2])
    cp |= 0x04;
  if (dots[3])
    cp |= 0x40;
  if (dots[4])
    cp |= 0x08;
  if (dots[5])
    cp |= 0x10;
  if (dots[6])
    cp |= 0x20;
  if (dots[7])
    cp |= 0x80;
  return cp;
}

// convert that braille codepoint to utf8 so we can actually print it
std::string Terminal::brailleToUTF8(u32 cp) {
  char buf[5];
  if (cp < 0x80) {
    buf[0] = cp;
    buf[1] = 0;
  } else if (cp < 0x800) {
    buf[0] = 0xC0 | (cp >> 6);
    buf[1] = 0x80 | (cp & 0x3F);
    buf[2] = 0;
  } else {
    buf[0] = 0xE0 | (cp >> 12);
    buf[1] = 0x80 | ((cp >> 6) & 0x3F);
    buf[2] = 0x80 | (cp & 0x3F);
    buf[3] = 0;
  }
  return std::string(buf);
}

std::string Terminal::toBraille(bool dots[8]) {
  return brailleToUTF8(brailleCodepoint(dots));
}

} // namespace jester
