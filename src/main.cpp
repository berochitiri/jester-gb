/* jester-gb: playing pokemon in a shitty terminal. built by bero. */

#include "apu/apu.hpp"
#include "bus/bus.hpp"
#include "cartridge/cartridge.hpp"
#include "cpu/cpu.hpp"
#include "input/input.hpp"
#include "ppu/ppu.hpp"
#include "tui/menu.hpp"
#include "tui/renderer.hpp"
#include "tui/terminal.hpp"
#include "types.hpp"

#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

using namespace jester;

static volatile bool running = true;

void signalHandler(int) { running = false; }

int main(int argc, char *argv[]) {
#ifdef _WIN32
  // fix windows shitty timer resolution (15ms -> 1ms)
  timeBeginPeriod(1);
#endif

  const char *directRomPath = nullptr;
  u8 argPalette = 255;
  int argVolume = -1;
  bool argDebug = false;
  bool useMenu = true;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      std::cerr << "jester-gb - Terminal Game Boy Emulator\n\n";
      std::cerr << "Usage: " << argv[0] << " [options] [rom.gb]\n\n";
      std::cerr << "Options:\n";
      std::cerr << "  -p <0-4>   Color palette\n";
      std::cerr << "  -v <0-100> Volume level\n";
      std::cerr << "  -d         Enable debug display\n";
      std::cerr << "  -h         Show help\n";
      return 0;
    } else if (strcmp(argv[i], "-d") == 0) {
      argDebug = true;
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      argPalette = std::atoi(argv[++i]);
      if (argPalette > 4)
        argPalette = 0;
    } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
      argVolume = std::atoi(argv[++i]);
      if (argVolume < 0)
        argVolume = 0;
      if (argVolume > 100)
        argVolume = 100;
    } else if (argv[i][0] != '-') {
      directRomPath = argv[i];
      useMenu = false;
    }
  }

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  Terminal terminal;
  Input input;
  Menu menu;
  APU apu;

  terminal.init();
  input.enableRawMode();
  menu.init(&terminal);
  menu.setAPU(&apu);

  std::string romPath;
  u8 palette;
  int volume;
  bool showDebug;

  if (useMenu) {
    romPath = menu.run();
    if (romPath.empty()) {
      input.disableRawMode();
      terminal.cleanup();
      return 0;
    }
    palette = menu.getPalette();
    volume = menu.getVolume();
    showDebug = menu.isDebugEnabled();
  } else {
    romPath = directRomPath;
    palette = (argPalette != 255) ? argPalette : 0;
    volume = (argVolume >= 0) ? argVolume : 50;
    showDebug = argDebug;
  }

  while (running && !romPath.empty()) {
    Bus bus;
    CPU cpu(bus);
    PPU ppu;
    Cartridge cartridge;

    if (!cartridge.load(romPath)) {
      input.disableRawMode();
      terminal.cleanup();
      std::cerr << "Failed to load ROM: " << romPath << "\n";
      return 1;
    }

    bus.attachCartridge(&cartridge);
    bus.attachPPU(&ppu);
    bus.attachAPU(&apu);
    bus.attachInput(&input);

    if (!apu.init()) { /* apu died? idc keep going */
    }
    apu.setVolume(volume);

    Renderer renderer;
    renderer.init(&terminal);
    renderer.setPalette(palette);

    printf("\033[2J");
    renderer.drawBorder();

    using Clock = std::chrono::high_resolution_clock;
    auto lastFpsTime = Clock::now();
    u32 frameCount = 0;
    double currentFps = 0.0;

    bool gameRunning = true;

    while (running && gameRunning) {
      auto frameStart = Clock::now();

      input.poll();

      if (input.shouldQuit()) {
        gameRunning = false;
        romPath = "";
        break;
      }

      if (input.shouldPause()) {
        input.clearPause();
        if (!menu.runPauseMenu(romPath)) {
          gameRunning = false;
          palette = menu.getPalette();
          volume = menu.getVolume();
          showDebug = menu.isDebugEnabled();
          apu.setVolume(volume);
          break;
        }
        palette = menu.getPalette();
        volume = menu.getVolume();
        showDebug = menu.isDebugEnabled();
        apu.setVolume(volume);
        renderer.setPalette(palette);
        renderer.drawBorder();
      }

      u32 frameCycles = 0;
      while (frameCycles < CYCLES_PER_FRAME) {
        u32 cycles = cpu.step();
        ppu.step(cycles);
        apu.step(cycles);
        frameCycles += cycles;

        if (ppu.hasVBlankInterrupt()) {
          cpu.requestInterrupt(INT_VBLANK);
          ppu.clearVBlankInterrupt();
        }
        if (ppu.hasStatInterrupt()) {
          cpu.requestInterrupt(INT_LCD);
          ppu.clearStatInterrupt();
        }
      }

      if (ppu.isFrameReady()) {
        renderer.render(ppu.getFrameBuffer());
        ppu.clearFrameReady();
        frameCount++;
      }

      auto now = Clock::now();
      auto fpsDelta = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - lastFpsTime);
      if (fpsDelta.count() >= 1000) {
        currentFps = frameCount * 1000.0 / fpsDelta.count();
        frameCount = 0;
        lastFpsTime = now;
      }

      if (showDebug) {
        renderer.renderDebug(cpu.getPC(), cpu.getA(), cpu.getF(), cpu.getSP(),
                             currentFps, cpu.getTotalCycles());
      }

      auto frameEnd = Clock::now();
      auto frameDuration =
          std::chrono::duration_cast<std::chrono::microseconds>(frameEnd -
                                                                frameStart);
      auto targetDuration =
          std::chrono::microseconds(static_cast<long>(FRAME_TIME_MS * 1000));

      if (frameDuration < targetDuration) {
        std::this_thread::sleep_for(targetDuration - frameDuration);
      }
    }

    cartridge.saveRAM();
    apu.cleanup();
  }

  input.disableRawMode();
  terminal.cleanup();

  std::cout << "\nbye bitch.\n";
#ifdef _WIN32
  timeEndPeriod(1);
#endif
  return 0;
}
