#include "input/input.hpp"
#include <cstring>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace jester {

#ifndef _WIN32
static struct termios origTermios;
#endif

Input::Input() = default;

Input::~Input() {
  if (rawModeEnabled) {
    disableRawMode();
  }
}

void Input::enableRawMode() {
#ifdef _WIN32
  // windows console setting shit
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(hStdin, &mode);
  mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
  SetConsoleMode(hStdin, mode);
#else
  tcgetattr(STDIN_FILENO, &origTermios);

  struct termios raw = origTermios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif

  rawModeEnabled = true;
}

void Input::disableRawMode() {
#ifdef _WIN32
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(hStdin, &mode);
  mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
  SetConsoleMode(hStdin, mode);
#else
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
#endif

  rawModeEnabled = false;
}

#ifndef _WIN32
static bool hasMoreInput() {
  struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
  return poll(&pfd, 1, 0) > 0;
}
#endif

void Input::poll() {
  std::memset(buttons, false, sizeof(buttons));

#ifdef _WIN32
  while (_kbhit()) {
    int c = _getch();
    if (c == 0 || c == 224) {
      c = _getch();
      switch (c) {
      case 72:
        buttons[BTN_UP] = true;
        break;
      case 80:
        buttons[BTN_DOWN] = true;
        break;
      case 75:
        buttons[BTN_LEFT] = true;
        break;
      case 77:
        buttons[BTN_RIGHT] = true;
        break;
      }
      continue;
    }
    switch (c) {
    case 'z':
    case 'Z':
      buttons[BTN_A] = true;
      break;
    case 'x':
    case 'X':
      buttons[BTN_B] = true;
      break;
    case '\r':
      buttons[BTN_START] = true;
      break;
    case ' ':
      buttons[BTN_SELECT] = true;
      break;
    case 27:
      pauseRequested = true;
      break;
    case 'q':
    case 'Q':
      quitRequested = true;
      break;
    }
  }
#else
  char c;
  while (::read(STDIN_FILENO, &c, 1) == 1) {
    if (c == 27) {
      if (hasMoreInput()) {
        char seq[2];
        if (::read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
          if (::read(STDIN_FILENO, &seq[1], 1) == 1) {
            switch (seq[1]) {
            case 'A':
              buttons[BTN_UP] = true;
              break;
            case 'B':
              buttons[BTN_DOWN] = true;
              break;
            case 'C':
              buttons[BTN_RIGHT] = true;
              break;
            case 'D':
              buttons[BTN_LEFT] = true;
              break;
            }
          }
        }
      } else {
        pauseRequested = true;
      }
      continue;
    }

    switch (c) {
    case 'w':
    case 'W':
      buttons[BTN_UP] = true;
      break;
    case 's':
    case 'S':
      buttons[BTN_DOWN] = true;
      break;
    case 'a':
    case 'A':
      buttons[BTN_LEFT] = true;
      break;
    case 'd':
    case 'D':
      buttons[BTN_RIGHT] = true;
      break;
    case 'z':
    case 'Z':
      buttons[BTN_A] = true;
      break;
    case 'x':
    case 'X':
      buttons[BTN_B] = true;
      break;
    case '\n':
    case '\r':
      buttons[BTN_START] = true;
      break;
    case ' ':
      buttons[BTN_SELECT] = true;
      break;
    case 'q':
    case 'Q':
      quitRequested = true;
      break;
    }
  }
#endif
}

u8 Input::read() const {
  u8 result = 0x0F;

  if (!(joypadSelect & 0x10)) {
    if (buttons[BTN_RIGHT])
      result &= ~0x01;
    if (buttons[BTN_LEFT])
      result &= ~0x02;
    if (buttons[BTN_UP])
      result &= ~0x04;
    if (buttons[BTN_DOWN])
      result &= ~0x08;
  }

  if (!(joypadSelect & 0x20)) {
    if (buttons[BTN_A])
      result &= ~0x01;
    if (buttons[BTN_B])
      result &= ~0x02;
    if (buttons[BTN_SELECT])
      result &= ~0x04;
    if (buttons[BTN_START])
      result &= ~0x08;
  }

  return result | (joypadSelect & 0x30) | 0xC0;
}

void Input::write(u8 val) { joypadSelect = val & 0x30; }

} // namespace jester
