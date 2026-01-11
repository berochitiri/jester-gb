#pragma once

#include "tui/terminal.hpp"
#include "types.hpp"
#include <string>
#include <vector>

namespace jester {

class APU;

class Menu {
public:
  Menu();

  void init(Terminal *term);
  void setAPU(APU *audio) { apu = audio; } // hook up the sound chip

  enum class State { MainMenu, Settings, RomBrowser, Playing, Paused, Quit };

  std::string run();
  bool runPauseMenu(std::string &currentRom);

  u8 getPalette() const { return palette; }
  int getVolume() const { return volume; }
  bool isDebugEnabled() const { return debugEnabled; }

  void loadSettings();
  void saveSettings();

private:
  Terminal *terminal = nullptr;
  APU *apu = nullptr;
  State state = State::MainMenu;

  u8 palette = 0;
  int volume = 50;
  bool debugEnabled = false;
  std::string romsPath = "roms";
  std::string configPath;

  int selection = 0;
  int scrollOffset = 0;
  std::vector<std::string> availableRoms;

  void findRomsDirectory();
  void setTerminalSize();

  void drawLogo();
  void drawMainMenu();
  void drawSettings();
  void drawRomBrowser();
  void drawPauseMenu();
  void drawFooter(const std::string &hint);
  void drawMenuItem(int x, int y, const std::string &text, bool selected);
  void drawSlider(int x, int y, const std::string &label, int value, int max,
                  bool selected);
  void clearArea(int x, int y, int w, int h);

  char waitForKey();
  void handleMainMenuInput(char key);
  void handleSettingsInput(char key);
  void handleRomBrowserInput(char key);
  bool handlePauseMenuInput(char key, std::string &currentRom);

  void scanForRoms();
  std::string promptForCustomRom();
};

} // namespace jester
