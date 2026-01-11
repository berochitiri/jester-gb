#include "bus/bus.hpp"
#include "apu/apu.hpp"
#include "cartridge/cartridge.hpp"
#include "input/input.hpp"
#include "ppu/ppu.hpp"

namespace jester {

Bus::Bus() {
  // wipe the ram clean
  wram.fill(0);
  hram.fill(0);
  ioRegs.fill(0);

  // hardware state post-boot (dmg style)
  ie = 0x00;
  div = 0xAB;
  tima = 0x00;
  tma = 0x00;
  tac = 0xF8;
  sb = 0x00;
  sc = 0x7E;

  // Initialize other I/O registers to post-boot values
  ioRegs[0x00] = 0xCF; // JOYP
  ioRegs[0x02] = 0x7E; // SC
  ioRegs[0x04] = 0xAB; // DIV
  ioRegs[0x07] = 0xF8; // TAC
  ioRegs[0x0F] = 0xE1; // IF
  ioRegs[0x40] = 0x91; // LCDC
  ioRegs[0x41] = 0x85; // STAT
  ioRegs[0x47] = 0xFC; // BGP
}

void Bus::attachCartridge(Cartridge *cart) { cartridge = cart; }

void Bus::attachPPU(PPU *p) { ppu = p; }

void Bus::attachInput(Input *i) { input = i; }

void Bus::attachAPU(APU *a) { apu = a; }

u8 Bus::read(u16 addr) {
  // first part of the game code (bank 0)
  if (addr <= 0x3FFF) {
    if (cartridge)
      return cartridge->read(addr);
    return 0xFF;
  }
  // the rest of the game code shit (bank n)
  else if (addr <= 0x7FFF) {
    if (cartridge)
      return cartridge->read(addr);
    return 0xFF;
  }
  // vram: where graphics live
  else if (addr <= 0x9FFF) {
    if (ppu)
      return ppu->readVRAM(addr - VRAM_START);
    return 0xFF;
  }
  // ext ram (for saves and shit)
  else if (addr <= 0xBFFF) {
    if (cartridge)
      return cartridge->read(addr);
    return 0xFF;
  }
  // wram: basic work ram shit
  else if (addr <= 0xDFFF) {
    return wram[addr - WRAM_START];
  }
  // ghost ram (mirrors wram)
  else if (addr <= 0xFDFF) {
    return wram[addr - ECHO_RAM_START];
  }
  // oam: sprite data shit
  else if (addr <= 0xFE9F) {
    if (ppu)
      return ppu->readOAM(addr - OAM_START);
    return 0xFF;
  }
  // trash zone (unusable)
  else if (addr <= 0xFEFF) {
    return 0xFF;
  }
  // talkin to hardware (io)
  else if (addr <= 0xFF7F) {
    return readIO(addr);
  }
  // high ram (fast shit)
  else if (addr <= 0xFFFE) {
    return hram[addr - HRAM_START];
  }
  // IE: tell the cpu which interrupts are chill
  else {
    return ie;
  }
}

void Bus::write(u16 addr, u8 val) {
  // cartridge handles its own bank swapping bullshit
  if (addr <= 0x7FFF) {
    if (cartridge)
      cartridge->write(addr, val);
  }
  // VRAM (0x8000-0x9FFF)
  else if (addr <= 0x9FFF) {
    if (ppu)
      ppu->writeVRAM(addr - VRAM_START, val);
  }
  // External RAM (0xA000-0xBFFF)
  else if (addr <= 0xBFFF) {
    if (cartridge)
      cartridge->write(addr, val);
  }
  // Work RAM (0xC000-0xDFFF)
  else if (addr <= 0xDFFF) {
    wram[addr - WRAM_START] = val;
  }
  // Echo RAM (0xE000-0xFDFF)
  else if (addr <= 0xFDFF) {
    wram[addr - ECHO_RAM_START] = val;
  }
  // OAM (0xFE00-0xFE9F)
  else if (addr <= 0xFE9F) {
    if (ppu)
      ppu->writeOAM(addr - OAM_START, val);
  }
  // Unusable (0xFEA0-0xFEFF)
  else if (addr <= 0xFEFF) {
    // dont do anything with it
  }
  // I/O Registers (0xFF00-0xFF7F)
  else if (addr <= 0xFF7F) {
    writeIO(addr, val);
  }
  // High RAM (0xFF80-0xFFFE)
  else if (addr <= 0xFFFE) {
    hram[addr - HRAM_START] = val;
  }
  // IE Register (0xFFFF)
  else {
    ie = val;
  }
}

u8 Bus::readIO(u16 addr) {
  switch (addr) {
  case JOYP:
    if (input)
      return input->read();
    return 0xFF;

  case 0xFF01:
    return sb; // serial data (wifi? lol nah)
  case 0xFF02:
    return sc; // Serial control

  case 0xFF04:
    return div; // DIV
  case 0xFF05:
    return tima; // TIMA
  case 0xFF06:
    return tma; // TMA
  case 0xFF07:
    return tac; // TAC

  case IF_REG:
    return ioRegs[0x0F] | 0xE0; // IF (upper bits always 1)

  // PPU registers
  case LCDC:
  case STAT:
  case SCY:
  case SCX:
  case LY:
  case LYC:
  case BGP:
  case OBP0:
  case OBP1:
  case WY:
  case WX:
    if (ppu)
      return ppu->readRegister(addr);
    return 0xFF;

  case DMA:
    return ioRegs[addr - IO_START];

  default:
    // APU registers (0xFF10-0xFF3F)
    if (addr >= 0xFF10 && addr <= 0xFF3F) {
      if (apu)
        return apu->readRegister(addr);
      return 0xFF;
    }
    if (addr >= IO_START && addr <= IO_END) {
      return ioRegs[addr - IO_START];
    }
    return 0xFF;
  }
}

void Bus::writeIO(u16 addr, u8 val) {
  switch (addr) {
  case JOYP:
    if (input)
      input->write(val);
    break;

  case 0xFF01:
    sb = val;
    break; // Serial data
  case 0xFF02:
    sc = val;
    break; // Serial control

  case 0xFF04:
    div = 0;
    break; // div: any write resets it back to zero lol
  case 0xFF05:
    tima = val;
    break; // TIMA
  case 0xFF06:
    tma = val;
    break; // TMA
  case 0xFF07:
    tac = val;
    break; // TAC

  case IF_REG:
    ioRegs[0x0F] = val & 0x1F;
    break; // IF

  // PPU registers
  case LCDC:
  case STAT:
  case SCY:
  case SCX:
  case LYC:
  case BGP:
  case OBP0:
  case OBP1:
  case WY:
  case WX:
    if (ppu)
      ppu->writeRegister(addr, val);
    break;

  case DMA:
    doDMATransfer(val);
    break;

  default:
    // APU registers (0xFF10-0xFF3F)
    if (addr >= 0xFF10 && addr <= 0xFF3F) {
      if (apu)
        apu->writeRegister(addr, val);
      break;
    }
    if (addr >= IO_START && addr <= IO_END) {
      ioRegs[addr - IO_START] = val;
    }
    break;
  }
}

void Bus::doDMATransfer(u8 val) {
  // dma: fast copy to oam cuz cpu is slow af
  u16 srcAddr = static_cast<u16>(val) << 8;
  for (u16 i = 0; i < 160; i++) {
    u8 data = read(srcAddr + i);
    if (ppu)
      ppu->writeOAM(i, data);
  }
}

u8 Bus::readDirect(u16 addr) const {
  // direct read for debugging (no side effects mfs)
  if (addr <= 0x3FFF || (addr >= 0x4000 && addr <= 0x7FFF)) {
    if (cartridge)
      return cartridge->readDirect(addr);
    return 0xFF;
  } else if (addr >= WRAM_START && addr <= WRAM_END) {
    return wram[addr - WRAM_START];
  } else if (addr >= HRAM_START && addr <= HRAM_END) {
    return hram[addr - HRAM_START];
  }
  return 0xFF;
}

} // namespace jester
