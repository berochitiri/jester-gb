#include "cartridge/cartridge.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace jester {

Cartridge::~Cartridge() {
  // saving your progress before this bitch closes (if battery exists)
  if (battery && ramDirty && !ram.empty()) {
    saveRAM();
  }
}

bool Cartridge::load(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return false;
  }

  // keep the path so we know where to save later
  romPath = path;

  // figure out where the .sav file goes (next to the rom)
  savePath = path;
  size_t dot = savePath.rfind('.');
  if (dot != std::string::npos) {
    savePath = savePath.substr(0, dot);
  }
  savePath += ".sav";

  // how big is this game?
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // suck in the rom data
  rom.resize(size);
  if (!file.read(reinterpret_cast<char *>(rom.data()), size)) {
    rom.clear();
    return false;
  }

  // check the game's bio (header)
  parseHeader();

  // how much ram for saves? check the codes
  static const u32 ramSizes[] = {0, 0, 0x2000, 0x8000, 0x20000, 0x10000};
  u8 ramSizeCode = (rom.size() > 0x149) ? rom[0x149] : 0;
  if (ramSizeCode < 6) {
    ram.resize(ramSizes[ramSizeCode], 0);
  }

  // reset all this mbc banking bullshit
  romBank = 1;
  ramBank = 0;
  ramEnabled = false;
  romRamMode = false;
  ramDirty = false;

  // load existing save if we have one (and it has a battery)
  if (battery && !ram.empty()) {
    loadRAM();
  }

  return true;
}

void Cartridge::parseHeader() {
  if (rom.size() < 0x150)
    return;

  // Title (0x134-0x143)
  title.clear();
  for (u16 i = 0x134; i < 0x144; i++) {
    char c = static_cast<char>(rom[i]);
    if (c == 0)
      break;
    title += c;
  }

  // what kind of mbc chip are we dealin with?
  mbcType = rom[0x147];

  // does this game even have a battery?
  switch (mbcType) {
  case 0x03: // MBC1+RAM+BATTERY
  case 0x06: // MBC2+BATTERY
  case 0x09: // ROM+RAM+BATTERY
  case 0x0D: // MMM01+RAM+BATTERY
  case 0x0F: // MBC3+TIMER+BATTERY
  case 0x10: // MBC3+TIMER+RAM+BATTERY
  case 0x13: // MBC3+RAM+BATTERY
  case 0x1B: // MBC5+RAM+BATTERY
  case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
  case 0x22: // MBC7+SENSOR+RUMBLE+RAM+BATTERY
  case 0xFF: // HuC1+RAM+BATTERY
    battery = true;
    break;
  default:
    battery = false;
  }
}

void Cartridge::saveRAM() {
  if (ram.empty() || savePath.empty())
    return;

  std::ofstream file(savePath, std::ios::binary);
  if (file) {
    file.write(reinterpret_cast<const char *>(ram.data()), ram.size());
    ramDirty = false;
  }
}

void Cartridge::loadRAM() {
  if (ram.empty() || savePath.empty())
    return;

  std::ifstream file(savePath, std::ios::binary);
  if (file) {
    file.read(reinterpret_cast<char *>(ram.data()), ram.size());
  }
}

u8 Cartridge::read(u16 addr) const { return readMBC(addr); }

void Cartridge::write(u16 addr, u8 val) { writeMBC(addr, val); }

u8 Cartridge::readDirect(u16 addr) const {
  if (addr < rom.size()) {
    return rom[addr];
  }
  return 0xFF;
}

u8 Cartridge::readMBC(u16 addr) const {
  // ROM Bank 0 (0x0000-0x3FFF)
  if (addr <= 0x3FFF) {
    if (addr < rom.size()) {
      return rom[addr];
    }
    return 0xFF;
  }
  // the rest of the rom (bank n) - banking logic ahead
  else if (addr <= 0x7FFF) {
    u32 bankAddr;

    switch (mbcType) {
    case 0x00: // No MBC
      bankAddr = addr;
      break;

    case 0x01: // MBC1
    case 0x02:
    case 0x03: {
      u8 bank = romBank;
      if (bank == 0)
        bank = 1;
      bankAddr = (addr - 0x4000) + (bank * 0x4000);
      break;
    }

    case 0x05: // MBC2
    case 0x06: {
      u8 bank = romBank & 0x0F;
      if (bank == 0)
        bank = 1;
      bankAddr = (addr - 0x4000) + (bank * 0x4000);
      break;
    }

    case 0x0F: // MBC3
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: {
      u8 bank = romBank;
      if (bank == 0)
        bank = 1;
      bankAddr = (addr - 0x4000) + (bank * 0x4000);
      break;
    }

    case 0x19: // MBC5
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E: {
      bankAddr = (addr - 0x4000) + (romBank * 0x4000);
      break;
    }

    default:
      bankAddr = addr;
      break;
    }

    if (bankAddr < rom.size()) {
      return rom[bankAddr];
    }
    return 0xFF;
  }
  // External RAM (0xA000-0xBFFF)
  else if (addr >= 0xA000 && addr <= 0xBFFF) {
    if (!ramEnabled || ram.empty()) {
      return 0xFF;
    }

    u32 ramAddr = (addr - 0xA000) + (ramBank * 0x2000);
    if (ramAddr < ram.size()) {
      return ram[ramAddr];
    }
    return 0xFF;
  }

  return 0xFF;
}

void Cartridge::writeMBC(u16 addr, u8 val) {
  // ram enable: 0x0A means "open the gate" mfs
  if (addr <= 0x1FFF) {
    switch (mbcType) {
    case 0x01:
    case 0x02:
    case 0x03: // MBC1
    case 0x05:
    case 0x06: // MBC2
    case 0x0F:
    case 0x10:
    case 0x11: // MBC3
    case 0x12:
    case 0x13:
    case 0x19:
    case 0x1A:
    case 0x1B: // MBC5
    case 0x1C:
    case 0x1D:
    case 0x1E:
      ramEnabled = ((val & 0x0F) == 0x0A);
      break;
    }
  }
  // ROM Bank Number (0x2000-0x3FFF)
  else if (addr <= 0x3FFF) {
    switch (mbcType) {
    case 0x01:
    case 0x02:
    case 0x03: // MBC1
      romBank = (romBank & 0x60) | (val & 0x1F);
      if ((romBank & 0x1F) == 0)
        romBank |= 1;
      break;

    case 0x05:
    case 0x06: // MBC2
      if (addr & 0x100) {
        romBank = val & 0x0F;
        if (romBank == 0)
          romBank = 1;
      }
      break;

    case 0x0F:
    case 0x10:
    case 0x11: // MBC3
    case 0x12:
    case 0x13:
      romBank = val & 0x7F;
      if (romBank == 0)
        romBank = 1;
      break;

    case 0x19:
    case 0x1A:
    case 0x1B: // MBC5
    case 0x1C:
    case 0x1D:
    case 0x1E:
      if (addr <= 0x2FFF) {
        romBank = (romBank & 0x100) | val;
      } else {
        romBank = (romBank & 0xFF) | ((val & 1) << 8);
      }
      break;
    }
  }
  // switch the ram bank or upper rom bank shit
  else if (addr <= 0x5FFF) {
    switch (mbcType) {
    case 0x01:
    case 0x02:
    case 0x03: // MBC1
      if (romRamMode) {
        ramBank = val & 0x03;
      } else {
        romBank = (romBank & 0x1F) | ((val & 0x03) << 5);
      }
      break;

    case 0x0F:
    case 0x10:
    case 0x11: // MBC3
    case 0x12:
    case 0x13:
      ramBank = val & 0x03;
      break;

    case 0x19:
    case 0x1A:
    case 0x1B: // MBC5
    case 0x1C:
    case 0x1D:
    case 0x1E:
      ramBank = val & 0x0F;
      break;
    }
  }
  // Banking Mode (0x6000-0x7FFF)
  else if (addr <= 0x7FFF) {
    switch (mbcType) {
    case 0x01:
    case 0x02:
    case 0x03: // MBC1
      romRamMode = (val & 0x01) != 0;
      break;
    }
  }
  // External RAM (0xA000-0xBFFF)
  else if (addr >= 0xA000 && addr <= 0xBFFF) {
    if (!ramEnabled || ram.empty())
      return;

    u32 ramAddr = (addr - 0xA000) + (ramBank * 0x2000);
    if (ramAddr < ram.size()) {
      ram[ramAddr] = val;
      ramDirty = true; // ram is nasty now, need to save later
    }
  }
}

} // namespace jester
