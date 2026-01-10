#pragma once

#include "types.hpp"

namespace jester {

class Bus;

class CPU {
public:
  explicit CPU(Bus &bus);

  u32 step();
  void reset();
  void requestInterrupt(u8 interrupt);

  u16 getPC() const { return pc; }
  u16 getSP() const { return sp; }
  u8 getA() const { return a; }
  u8 getF() const { return f; }
  u64 getTotalCycles() const { return totalCycles; }

private:
  Bus &bus;

  // data buckets (registers)
  u8 a = 0, f = 0;
  u8 b = 0, c = 0;
  u8 d = 0, e = 0;
  u8 h = 0, l = 0;
  u16 sp = 0;
  u16 pc = 0;

  // current vibe (state)
  bool ime = false;
  bool halted = false;
  bool stopped = false;
  bool imeScheduled = false;
  u64 totalCycles = 0;

  // flags: for when shit happens
  bool getZ() const { return (f & 0x80) != 0; }
  bool getN() const { return (f & 0x40) != 0; }
  bool getH() const { return (f & 0x20) != 0; }
  bool getC() const { return (f & 0x10) != 0; }

  void setZ(bool v) { f = v ? (f | 0x80) : (f & ~0x80); }
  void setN(bool v) { f = v ? (f | 0x40) : (f & ~0x40); }
  void setH(bool v) { f = v ? (f | 0x20) : (f & ~0x20); }
  void setC(bool v) { f = v ? (f | 0x10) : (f & ~0x10); }

  // big buckets (16-bit pairs)
  u16 getAF() const { return (u16(a) << 8) | f; }
  u16 getBC() const { return (u16(b) << 8) | c; }
  u16 getDE() const { return (u16(d) << 8) | e; }
  u16 getHL() const { return (u16(h) << 8) | l; }

  void setAF(u16 v) {
    a = v >> 8;
    f = v & 0xF0;
  }
  void setBC(u16 v) {
    b = v >> 8;
    c = v & 0xFF;
  }
  void setDE(u16 v) {
    d = v >> 8;
    e = v & 0xFF;
  }
  void setHL(u16 v) {
    h = v >> 8;
    l = v & 0xFF;
  }

  // talkin to the memory bus
  u8 read8(u16 addr);
  void write8(u16 addr, u8 val);
  u16 read16(u16 addr);
  void write16(u16 addr, u16 val);
  u8 fetchByte();
  u16 fetchWord();
  void push16(u16 val);
  u16 pop16();

  // calculator shit
  void add8(u8 val);
  void adc8(u8 val);
  void sub8(u8 val);
  void sbc8(u8 val);
  void and8(u8 val);
  void or8(u8 val);
  void xor8(u8 val);
  void cp8(u8 val);
  u8 inc8(u8 val);
  u8 dec8(u8 val);
  void addHL(u16 val);
  void addSP(s8 val);

  // spinnin bits
  u8 rlc(u8 val);
  u8 rrc(u8 val);
  u8 rl(u8 val);
  u8 rr(u8 val);
  u8 sla(u8 val);
  u8 sra(u8 val);
  u8 swap(u8 val);
  u8 srl(u8 val);
  void bit(u8 n, u8 val);
  u8 res(u8 n, u8 val);
  u8 set(u8 n, u8 val);

  // teleportin around (jumps)
  void jr(bool condition);
  void jp(bool condition);
  void call(bool condition);
  void ret(bool condition);
  void rst(u8 vec);

  void handleInterrupts();
  u32 executeOpcode(u8 opcode);
  u32 executeCBOpcode(u8 opcode);
};

} // namespace jester
