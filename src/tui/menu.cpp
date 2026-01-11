#include "tui/menu.hpp"
#include "apu/apu.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <conio.h>
#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace jester {

Menu::Menu() {
  // where is the config shit?
#ifdef _WIN32
  const char *appdata = getenv("APPDATA");
  if (appdata) {
    configPath = std::string(appdata) + "\\jester-gb\\config.ini";
  } else {
    configPath = "jester-gb.ini";
  }
#else
  const char *home = getenv("HOME");
  if (home) {
    configPath = std::string(home) + "/.config/jester-gb/config";
  } else {
    configPath = ".jester-gb.conf";
  }
#endif

  // hunt for the roms folder near the app
  findRomsDirectory();
}

void Menu::findRomsDirectory() {
  // searchin all over for those games
  const char *romsDirs[] = {"roms",     "../roms", "./roms",
#ifdef _WIN32
                            "..\\roms", ".\\roms",
#endif
                            nullptr};

  for (int i = 0; romsDirs[i]; i++) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(romsDirs[i]);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
      romsPath = romsDirs[i];
      return;
    }
#else
    struct stat st;
    if (stat(romsDirs[i], &st) == 0 && S_ISDIR(st.st_mode)) {
      romsPath = romsDirs[i];
      return;
    }
#endif
  }
  romsPath = "roms"; // fuck it, just assume its in roms/
}

void Menu::init(Terminal *term) {
  terminal = term;
  loadSettings();
  setTerminalSize();
}

void Menu::setTerminalSize() {
  // set the terminal title so it looks cool
#ifdef _WIN32
  SetConsoleTitleA("jester-gb");
#else
  printf("\033]0;jester-gb\007");
  fflush(stdout);
#endif
}

void Menu::loadSettings() {
  std::ifstream file(configPath);
  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line)) {
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;

    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    if (key == "palette")
      palette = std::stoi(val);
    else if (key == "volume")
      volume = std::stoi(val);
    else if (key == "debug")
      debugEnabled = (val == "1");
    else if (key == "roms_path" && !val.empty())
      romsPath = val;
  }
}

void Menu::saveSettings() {
  // make sure the folder actually exists lol
  std::string dir = configPath.substr(0, configPath.rfind('/'));
#ifdef _WIN32
  dir = configPath.substr(0, configPath.rfind('\\'));
  _mkdir(dir.c_str());
#else
  mkdir(dir.c_str(), 0755);
#endif

  std::ofstream file(configPath);
  if (!file.is_open())
    return;

  file << "palette=" << (int)palette << "\n";
  file << "volume=" << volume << "\n";
  file << "debug=" << (debugEnabled ? "1" : "0") << "\n";
  file << "roms_path=" << romsPath << "\n";
}

char Menu::waitForKey() {
#ifdef _WIN32
  // windows console input shit
  int c = _getch();
  if (c == 0 || c == 224) { // some weird long key press (arrow keys etc)
    c = _getch();
    switch (c) {
    case 72:
      return 'w'; // Up
    case 80:
      return 's'; // Down
    case 75:
      return 'a'; // Left
    case 77:
      return 'd'; // Right
    }
    return 0;
  }
  if (c == 27)
    return 27; // ESC
  return (char)c;
#else
  char c = 0;
  while (::read(STDIN_FILENO, &c, 1) != 1) {
    usleep(10000);
  }

  // handle escape sequences (mostly arrow keys)
  if (c == 27) {
    char seq[2] = {0, 0};
    if (::read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
      if (::read(STDIN_FILENO, &seq[1], 1) == 1) {
        switch (seq[1]) {
        case 'A':
          return 'w';
        case 'B':
          return 's';
        case 'C':
          return 'd';
        case 'D':
          return 'a';
        }
      }
    }
    return 27;
  }
  return c;
#endif
}

void Menu::clearArea(int x, int y, int w, int h) {
  std::string spaces(w, ' ');
  for (int i = 0; i < h; i++) {
    printf("\033[%d;%dH%s", y + i, x, spaces.c_str());
  }
}

void Menu::drawLogo() {
  printf("\033[2J\033[H");

  // cool ascii art logo i found
  printf("\033[38;2;100;255;100m");
  printf("\033[2;15H     _           _                       _     ");
  printf("\033[3;15H    (_) ___  ___| |_ ___ _ __      __ _| |__  ");
  printf("\033[4;15H    | |/ _ \\/ __| __/ _ \\ '__|____/ _` | '_ \\ ");
  printf("\033[5;15H    | |  __/\\__ \\ ||  __/ | |____| (_| | |_) |");
  printf("\033[6;15H   _/ |\\___||___/\\__\\___|_|       \\__, |_.__/ ");
  printf("\033[7;15H  |__/                            |___/       ");
  printf("\033[0m");

  printf("\033[9;22H\033[38;2;80;80;80mTerminal Game Boy Emulator\033[0m");
}

void Menu::drawFooter(const std::string &hint) {
  printf("\033[24;2H\033[38;2;60;60;60m%s\033[K\033[0m", hint.c_str());
}

void Menu::drawMenuItem(int x, int y, const std::string &text, bool selected) {
  printf("\033[%d;%dH", y, x);

  if (selected) {
    printf("\033[38;2;0;0;0m\033[48;2;100;255;100m");
    printf(" > %s ", text.c_str());
  } else {
    printf("\033[38;2;150;150;150m");
    printf("   %s  ", text.c_str());
  }
  printf("\033[0m");
}

void Menu::drawSlider(int x, int y, const std::string &label, int value,
                      int max, bool selected) {
  printf("\033[%d;%dH", y, x);

  if (selected) {
    printf("\033[38;2;100;255;100m");
  } else {
    printf("\033[38;2;150;150;150m");
  }

  printf("%s: ", label.c_str());

  int barWidth = 15;
  int fillWidth = (value * barWidth) / max;

  printf("[");
  printf("\033[38;2;100;255;100m");
  for (int i = 0; i < fillWidth; i++)
    printf("#");
  printf("\033[38;2;40;40;40m");
  for (int i = fillWidth; i < barWidth; i++)
    printf("-");
  printf("\033[0m] %3d", value);

  if (selected) {
    printf(" \033[38;2;80;80;80m< >\033[0m");
  }
}

void Menu::drawMainMenu() {
  drawLogo();

  printf("\033[11;28H\033[38;2;100;255;100m[ MAIN MENU ]\033[0m");

  drawMenuItem(26, 13, "Start Game     ", selection == 0);
  drawMenuItem(26, 15, "Settings       ", selection == 1);
  drawMenuItem(26, 17, "Quit           ", selection == 2);

  drawFooter("Arrow Keys: Navigate   Enter: Select   Q: Quit");
  fflush(stdout);
}

void Menu::drawSettings() {
  drawLogo();

  printf("\033[11;28H\033[38;2;100;255;100m[ SETTINGS ]\033[0m");

  drawSlider(20, 13, "Volume  ", volume, 100, selection == 0);

  // Palette
  printf("\033[15;20H");
  if (selection == 1)
    printf("\033[38;2;100;255;100m");
  else
    printf("\033[38;2;150;150;150m");

  static const char *palettes[] = {"White", "Green", "Amber", "Blue ", "Pink "};
  printf("Palette : %s", palettes[palette]);
  if (selection == 1)
    printf(" \033[38;2;80;80;80m< >\033[0m");

  // Debug
  printf("\033[17;20H");
  if (selection == 2)
    printf("\033[38;2;100;255;100m");
  else
    printf("\033[38;2;150;150;150m");
  printf("Debug   : %s\033[0m", debugEnabled ? "[ON] " : "[OFF]");

  drawMenuItem(26, 20, "Back           ", selection == 3);

  drawFooter("Arrow Keys: Navigate   < >: Adjust   Enter: Select   Esc: Back");
  fflush(stdout);
}

void Menu::drawRomBrowser() {
  drawLogo();

  printf("\033[11;26H\033[38;2;100;255;100m[ SELECT ROM ]\033[0m");

  int startY = 13;
  int maxVisible = 6;

  if (availableRoms.empty()) {
    printf("\033[14;18H\033[38;2;100;100;100mNo ROMs found in %s\033[0m",
           romsPath.c_str());
    drawMenuItem(23, 16, "Browse for ROM...", selection == 0);
    drawMenuItem(23, 18, "Back             ", selection == 1);
  } else {
    if (selection < scrollOffset)
      scrollOffset = selection;
    if (selection >= scrollOffset + maxVisible)
      scrollOffset = selection - maxVisible + 1;

    int endIdx = std::min((int)availableRoms.size(), scrollOffset + maxVisible);

    for (int i = scrollOffset; i < endIdx; i++) {
      std::string name = availableRoms[i];
      size_t slash = name.rfind('/');
#ifdef _WIN32
      size_t bslash = name.rfind('\\');
      if (bslash != std::string::npos &&
          (slash == std::string::npos || bslash > slash))
        slash = bslash;
#endif
      if (slash != std::string::npos)
        name = name.substr(slash + 1);
      if (name.length() > 30)
        name = name.substr(0, 27) + "...";
      while (name.length() < 30)
        name += ' ';

      drawMenuItem(18, startY + (i - scrollOffset) * 2, name, selection == i);
    }

    int extraY = startY + maxVisible * 2 + 1;
    drawMenuItem(23, extraY, "Browse for ROM...",
                 selection == (int)availableRoms.size());
    drawMenuItem(23, extraY + 2, "Back             ",
                 selection == (int)availableRoms.size() + 1);
  }

  drawFooter("Arrow Keys: Navigate   Enter: Select   Esc: Back");
  fflush(stdout);
}

void Menu::drawPauseMenu() {
  printf("\033[8;18H\033[48;2;20;20;20m");
  printf("+------------------------------------+");
  for (int y = 9; y < 22; y++) {
    printf("\033[%d;18H|                                    |", y);
  }
  printf("\033[22;18H+------------------------------------+");
  printf("\033[0m");

  printf("\033[10;30H\033[38;2;100;255;100m[ PAUSED ]\033[0m");

  drawMenuItem(28, 12, "Resume        ", selection == 0);
  drawSlider(26, 14, "Volume", volume, 100, selection == 1);

  printf("\033[16;26H");
  if (selection == 2)
    printf("\033[38;2;100;255;100m");
  else
    printf("\033[38;2;150;150;150m");
  static const char *paletteNames[] = {"White", "Green", "Amber", "Blue",
                                       "Pink"};
  printf("Palette: %s", paletteNames[palette]);
  if (selection == 2)
    printf(" < >");
  printf("\033[0m");

  drawMenuItem(28, 18, "Change ROM    ", selection == 3);
  drawMenuItem(28, 19, "Main Menu     ", selection == 4);
  drawMenuItem(28, 20, "Quit          ", selection == 5);

  printf("\033[24;20H\033[38;2;60;60;60mEsc: Resume   Arrows: Navigate\033[0m");
  fflush(stdout);
}

void Menu::scanForRoms() {
  availableRoms.clear();
  scrollOffset = 0;

#ifdef _WIN32
  WIN32_FIND_DATAA ffd;
  std::string searchPath = romsPath + "\\*.gb";
  HANDLE hFind = FindFirstFileA(searchPath.c_str(), &ffd);

  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        availableRoms.push_back(romsPath + "\\" + ffd.cFileName);
      }
    } while (FindNextFileA(hFind, &ffd));
    FindClose(hFind);
  }

  // check for .gbc games too
  searchPath = romsPath + "\\*.gbc";
  hFind = FindFirstFileA(searchPath.c_str(), &ffd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        availableRoms.push_back(romsPath + "\\" + ffd.cFileName);
      }
    } while (FindNextFileA(hFind, &ffd));
    FindClose(hFind);
  }
#else
  DIR *dir = opendir(romsPath.c_str());
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;
    if (name.length() > 3) {
      std::string ext = name.substr(name.length() - 3);
      for (char &c : ext)
        c = tolower(c);
      if (ext == ".gb" ||
          (name.length() > 4 && name.substr(name.length() - 4) == ".gbc")) {
        availableRoms.push_back(romsPath + "/" + name);
      }
    }
  }
  closedir(dir);
#endif

  std::sort(availableRoms.begin(), availableRoms.end());
}

std::string Menu::promptForCustomRom() {
#ifdef _WIN32
  // open that windows file picker shit
  char filename[MAX_PATH] = "";
  OPENFILENAMEA ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = "Game Boy ROMs\0*.gb;*.gbc\0All Files\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = "Select ROM";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

  if (GetOpenFileNameA(&ofn)) {
    return std::string(filename);
  }
  return "";
#else
  // linux file picker BULLSHIT (zenity/kdialog)
  printf("\033[?1049l");
  fflush(stdout);

  FILE *pipe = popen(
      "zenity --file-selection --file-filter='Game Boy|*.gb *.gbc *.GB *.GBC' "
      "--title='Select ROM' 2>/dev/null || "
      "kdialog --getopenfilename ~ '*.gb *.gbc' 2>/dev/null",
      "r");

  std::string result;
  if (pipe) {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe))
      result += buffer;
    pclose(pipe);
  }

  printf("\033[?1049h");
  fflush(stdout);

  while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
    result.pop_back();

  return result;
#endif
}

void Menu::handleMainMenuInput(char key) {
  switch (key) {
  case 'w':
  case 'W':
    selection = (selection + 2) % 3;
    break;
  case 's':
  case 'S':
    selection = (selection + 1) % 3;
    break;
  case '\n':
  case '\r':
    if (selection == 0) {
      state = State::RomBrowser;
      selection = 0;
      scanForRoms();
    } else if (selection == 1) {
      state = State::Settings;
      selection = 0;
    } else {
      state = State::Quit;
    }
    break;
  case 'q':
  case 'Q':
  case 27:
    state = State::Quit;
    break;
  }
}

void Menu::handleSettingsInput(char key) {
  switch (key) {
  case 'w':
  case 'W':
    selection = (selection + 3) % 4;
    break;
  case 's':
  case 'S':
    selection = (selection + 1) % 4;
    break;
  case 'a':
  case 'A':
    if (selection == 0 && volume > 0) {
      volume -= 5;
      if (apu)
        apu->setVolume(volume);
    }
    if (selection == 1 && palette > 0)
      palette--;
    break;
  case 'd':
  case 'D':
    if (selection == 0 && volume < 100) {
      volume += 5;
      if (apu)
        apu->setVolume(volume);
    }
    if (selection == 1 && palette < 4)
      palette++;
    break;
  case '\n':
  case '\r':
    if (selection == 2)
      debugEnabled = !debugEnabled;
    if (selection == 3) {
      state = State::MainMenu;
      selection = 0;
      saveSettings();
    }
    break;
  case 27:
    state = State::MainMenu;
    selection = 0;
    saveSettings();
    break;
  }
}

void Menu::handleRomBrowserInput(char key) {
  int total = availableRoms.size() + 2;

  switch (key) {
  case 'w':
  case 'W':
    selection = (selection + total - 1) % total;
    break;
  case 's':
  case 'S':
    selection = (selection + 1) % total;
    break;
  case '\n':
  case '\r':
    if (selection < (int)availableRoms.size()) {
      state = State::Playing;
    } else if (selection == (int)availableRoms.size()) {
      std::string path = promptForCustomRom();
      if (!path.empty()) {
        availableRoms.push_back(path);
        selection = availableRoms.size() - 1;
        state = State::Playing;
      }
    } else {
      state = State::MainMenu;
      selection = 0;
    }
    break;
  case 27:
    state = State::MainMenu;
    selection = 0;
    break;
  }
}

bool Menu::handlePauseMenuInput(char key, std::string &currentRom) {
  switch (key) {
  case 'w':
  case 'W':
    selection = (selection + 5) % 6;
    return false;
  case 's':
  case 'S':
    selection = (selection + 1) % 6;
    return false;
  case 'a':
  case 'A':
    if (selection == 1 && volume > 0) {
      volume -= 5;
      if (apu)
        apu->setVolume(volume);
    }
    if (selection == 2 && palette > 0)
      palette--;
    return false;
  case 'd':
  case 'D':
    if (selection == 1 && volume < 100) {
      volume += 5;
      if (apu)
        apu->setVolume(volume);
    }
    if (selection == 2 && palette < 4)
      palette++;
    return false;
  case '\n':
  case '\r':
    switch (selection) {
    case 0:
      return true; // Resume
    case 3:
      scanForRoms();
      state = State::RomBrowser;
      selection = 0;
      currentRom = "";
      return true;
    case 4:
      state = State::MainMenu;
      selection = 0;
      currentRom = "";
      return true;
    case 5:
      state = State::Quit;
      return true;
    }
    return false;
  case 27:
    return true; // ESC = resume
  default:
    return false;
  }
}

std::string Menu::run() {
  if (!terminal)
    return "";

  state = State::MainMenu;
  selection = 0;

  printf("\033[?1049h"); // swap to alternate screen mfs
  printf("\033[?25l");   // Hide cursor
  fflush(stdout);

  while (state != State::Quit && state != State::Playing) {
    switch (state) {
    case State::MainMenu:
      drawMainMenu();
      handleMainMenuInput(waitForKey());
      break;
    case State::Settings:
      drawSettings();
      handleSettingsInput(waitForKey());
      break;
    case State::RomBrowser:
      drawRomBrowser();
      handleRomBrowserInput(waitForKey());
      break;
    default:
      break;
    }
  }

  printf("\033[2J");
  fflush(stdout);

  if (state == State::Playing && selection < (int)availableRoms.size()) {
    saveSettings();
    return availableRoms[selection];
  }

  printf("\033[?1049l");
  printf("\033[?25h");
  return "";
}

bool Menu::runPauseMenu(std::string &currentRom) {
  selection = 0;
  state = State::Paused;

  while (state == State::Paused) {
    drawPauseMenu();
    char key = waitForKey();

    bool shouldExit = handlePauseMenuInput(key, currentRom);

    if (shouldExit) {
      if (state == State::Paused) {
        saveSettings();
        printf("\033[2J");
        return true; // Resume
      }
    }
  }

  saveSettings();

  if (state == State::Quit) {
    currentRom = ""; // nuke the rom path so we can dip
    return false;
  }

  if (state == State::MainMenu || state == State::RomBrowser) {
    std::string newRom = run();
    if (newRom.empty()) {
      currentRom = ""; // user quit from the menu like a loser
      return false;
    }
    currentRom = newRom;
    return false; // Return false to break inner loop and reload ROM
  }

  return true;
}

} // namespace jester
