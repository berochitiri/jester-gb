#pragma once

#include "types.hpp"
#include <array>
#include <string>
#include <vector>

namespace jester {

class Cartridge {
public:
  Cartridge() = default;
  ~Cartridge();

  bool load(const std::string &path);
  u8 read(u16 addr) const;
  void write(u16 addr, u8 val);
  u8 readDirect(u16 addr) const;

  void saveRAM();
  void loadRAM();

  std::string getTitle() const { return title; }
  u8 getMBCType() const { return mbcType; }
  u32 getROMSize() const { return rom.size(); }
  u32 getRAMSize() const { return ram.size(); }
  bool isLoaded() const { return !rom.empty(); }
  bool hasBattery() const { return battery; }

private:
  std::vector<u8> rom;
  std::vector<u8> ram;
  std::string title;
  std::string romPath;
  std::string savePath;
  u8 mbcType = 0;
  bool battery = false; // logic for keeping saves alive

  u8 romBank = 1;
  u8 ramBank = 0;
  bool ramEnabled = false;
  bool romRamMode = false;
  bool ramDirty = false;

  void parseHeader();
  u8 readMBC(u16 addr) const;
  void writeMBC(u16 addr, u8 val);
};

} // namespace jester
