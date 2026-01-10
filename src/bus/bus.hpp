#pragma once

#include "types.hpp"
#include <array>

namespace jester {

class Cartridge;
class PPU;
class Input;
class APU;

class Bus {
public:
  Bus();

  void attachCartridge(Cartridge *cart);
  void attachPPU(PPU *ppu);
  void attachInput(Input *input);
  void attachAPU(APU *apu);

  u8 read(u16 addr);
  void write(u16 addr, u8 val);
  void doDMATransfer(u8 val);
  u8 readDirect(u16 addr) const; // read without side effects (debug mode only)

private:
  std::array<u8, 0x2000> wram;
  std::array<u8, 0x7F> hram;
  std::array<u8, 0x80> ioRegs;

  u8 ie;
  Cartridge *cartridge = nullptr;
  PPU *ppu = nullptr;
  Input *input = nullptr;
  APU *apu = nullptr;

  u8 div, tima, tma, tac;
  u8 sb, sc;

  u8 readIO(u16 addr);
  void writeIO(u16 addr, u8 val);
};

} // namespace jester
