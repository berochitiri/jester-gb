#pragma once

#include "types.hpp"

namespace jester {

class Input {
public:
  Input();
  ~Input();

  void enableRawMode();
  void disableRawMode();
  void poll();

  u8 read() const;
  void write(u8 val);

  bool shouldQuit() const { return quitRequested; }
  bool shouldPause() const { return pauseRequested; }
  void clearPause() { pauseRequested = false; }
  void clearQuit() { quitRequested = false; }

private:
  bool rawModeEnabled = false;
  bool buttons[8] = {false};
  u8 joypadSelect = 0;
  bool quitRequested = false;
  bool pauseRequested = false;

  static constexpr u8 BTN_RIGHT = 0;
  static constexpr u8 BTN_LEFT = 1;
  static constexpr u8 BTN_UP = 2;
  static constexpr u8 BTN_DOWN = 3;
  static constexpr u8 BTN_A = 4;
  static constexpr u8 BTN_B = 5;
  static constexpr u8 BTN_SELECT = 6;
  static constexpr u8 BTN_START = 7;
};

} // namespace jester
