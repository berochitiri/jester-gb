#include "cpu/cpu.hpp"
#include "bus/bus.hpp"
#include "cpu/opcodes.hpp"

namespace jester {

CPU::CPU(Bus &bus) : bus(bus) { reset(); }

void CPU::reset() {
  // waking up this brain dead dmg cpu
  a = 0x01;
  f = 0xB0; // Z=1, N=0, H=1, C=1
  b = 0x00;
  c = 0x13;
  d = 0x00;
  e = 0xD8;
  h = 0x01;
  l = 0x4D;
  sp = 0xFFFE;
  pc = 0x0100; // entry point post bootrom mfs

  ime = false;
  imeScheduled = false;
  halted = false;
  stopped = false;
  totalCycles = 0;
}

u8 CPU::read8(u16 addr) { return bus.read(addr); }

void CPU::write8(u16 addr, u8 val) { bus.write(addr, val); }

u16 CPU::read16(u16 addr) {
  u8 lo = read8(addr);
  u8 hi = read8(addr + 1);
  return (static_cast<u16>(hi) << 8) | lo;
}

void CPU::write16(u16 addr, u16 val) {
  write8(addr, val & 0xFF);
  write8(addr + 1, val >> 8);
}

u8 CPU::fetchByte() { return read8(pc++); }

u16 CPU::fetchWord() {
  u16 val = read16(pc);
  pc += 2;
  return val;
}

void CPU::push16(u16 val) {
  sp -= 2;
  write16(sp, val);
}

u16 CPU::pop16() {
  u16 val = read16(sp);
  sp += 2;
  return val;
}

// math shit
void CPU::add8(u8 val) {
  u16 result = a + val;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(((a & 0x0F) + (val & 0x0F)) > 0x0F);
  setC(result > 0xFF);
  a = result & 0xFF;
}

void CPU::adc8(u8 val) {
  u8 carry = getC() ? 1 : 0;
  u16 result = a + val + carry;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(((a & 0x0F) + (val & 0x0F) + carry) > 0x0F);
  setC(result > 0xFF);
  a = result & 0xFF;
}

void CPU::sub8(u8 val) {
  setZ(a == val);
  setN(true);
  setH((a & 0x0F) < (val & 0x0F));
  setC(a < val);
  a -= val;
}

void CPU::sbc8(u8 val) {
  u8 carry = getC() ? 1 : 0;
  s16 result = a - val - carry;
  setZ((result & 0xFF) == 0);
  setN(true);
  setH(((a & 0x0F) - (val & 0x0F) - carry) < 0);
  setC(result < 0);
  a = result & 0xFF;
}

void CPU::and8(u8 val) {
  a &= val;
  setZ(a == 0);
  setN(false);
  setH(true);
  setC(false);
}

void CPU::or8(u8 val) {
  a |= val;
  setZ(a == 0);
  setN(false);
  setH(false);
  setC(false);
}

void CPU::xor8(u8 val) {
  a ^= val;
  setZ(a == 0);
  setN(false);
  setH(false);
  setC(false);
}

void CPU::cp8(u8 val) {
  setZ(a == val);
  setN(true);
  setH((a & 0x0F) < (val & 0x0F));
  setC(a < val);
}

u8 CPU::inc8(u8 val) {
  u8 result = val + 1;
  setZ(result == 0);
  setN(false);
  setH((val & 0x0F) == 0x0F);
  return result;
}

u8 CPU::dec8(u8 val) {
  u8 result = val - 1;
  setZ(result == 0);
  setN(true);
  setH((val & 0x0F) == 0);
  return result;
}

void CPU::addHL(u16 val) {
  u16 hl = getHL();
  u32 result = hl + val;
  setN(false);
  setH(((hl & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF);
  setC(result > 0xFFFF);
  setHL(result & 0xFFFF);
}

void CPU::addSP(s8 val) {
  u16 result = sp + val;
  setZ(false);
  setN(false);
  setH(((sp & 0x0F) + (val & 0x0F)) > 0x0F);
  setC(((sp & 0xFF) + (val & 0xFF)) > 0xFF);
  sp = result;
}

// rotate and shift bullshit
u8 CPU::rlc(u8 val) {
  u8 carry = (val >> 7) & 1;
  u8 result = (val << 1) | carry;
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(carry);
  return result;
}

u8 CPU::rrc(u8 val) {
  u8 carry = val & 1;
  u8 result = (val >> 1) | (carry << 7);
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(carry);
  return result;
}

u8 CPU::rl(u8 val) {
  u8 oldCarry = getC() ? 1 : 0;
  u8 newCarry = (val >> 7) & 1;
  u8 result = (val << 1) | oldCarry;
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(newCarry);
  return result;
}

u8 CPU::rr(u8 val) {
  u8 oldCarry = getC() ? 0x80 : 0;
  u8 newCarry = val & 1;
  u8 result = (val >> 1) | oldCarry;
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(newCarry);
  return result;
}

u8 CPU::sla(u8 val) {
  u8 carry = (val >> 7) & 1;
  u8 result = val << 1;
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(carry);
  return result;
}

u8 CPU::sra(u8 val) {
  u8 carry = val & 1;
  u8 result = (val >> 1) | (val & 0x80); // keep bit 7
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(carry);
  return result;
}

u8 CPU::swap(u8 val) {
  u8 result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(false);
  return result;
}

u8 CPU::srl(u8 val) {
  u8 carry = val & 1;
  u8 result = val >> 1;
  setZ(result == 0);
  setN(false);
  setH(false);
  setC(carry);
  return result;
}

// flippin bits
void CPU::bit(u8 n, u8 val) {
  setZ(!((val >> n) & 1));
  setN(false);
  setH(true);
}

u8 CPU::res(u8 n, u8 val) { return val & ~(1 << n); }

u8 CPU::set(u8 n, u8 val) { return val | (1 << n); }

// jumps and calls lol
void CPU::jr(bool condition) {
  s8 offset = static_cast<s8>(fetchByte());
  if (condition) {
    pc += offset;
  }
}

void CPU::jp(bool condition) {
  u16 addr = fetchWord();
  if (condition) {
    pc = addr;
  }
}

void CPU::call(bool condition) {
  u16 addr = fetchWord();
  if (condition) {
    push16(pc);
    pc = addr;
  }
}

void CPU::ret(bool condition) {
  if (condition) {
    pc = pop16();
  }
}

void CPU::rst(u8 vec) {
  push16(pc);
  pc = vec;
}

// ask for attention (interrupts)
void CPU::requestInterrupt(u8 interrupt) {
  u8 ifReg = read8(IF_REG);
  write8(IF_REG, ifReg | interrupt);
}

// dealing with interrupts
void CPU::handleInterrupts() {
  if (!ime && !halted)
    return;

  u8 ifReg = read8(IF_REG);
  u8 ieReg = read8(IE_REG);
  u8 pending = ifReg & ieReg & 0x1F;

  if (pending == 0)
    return;

  halted = false;

  if (!ime)
    return;

  ime = false;

  // priorities: vblank > lcd > timer > serial > joypad (vblank is king)
  u16 vector = 0;
  u8 bit = 0;

  if (pending & INT_VBLANK) {
    vector = 0x0040;
    bit = INT_VBLANK;
  } else if (pending & INT_LCD) {
    vector = 0x0048;
    bit = INT_LCD;
  } else if (pending & INT_TIMER) {
    vector = 0x0050;
    bit = INT_TIMER;
  } else if (pending & INT_SERIAL) {
    vector = 0x0058;
    bit = INT_SERIAL;
  } else if (pending & INT_JOYPAD) {
    vector = 0x0060;
    bit = INT_JOYPAD;
  }

  // clear the flag
  write8(IF_REG, ifReg & ~bit);

  // push pc and jump to the vector
  push16(pc);
  pc = vector;
}

// keep this bitch movin
u32 CPU::step() {
  // handle Scheduled ime enable (for ei instruction)
  if (imeScheduled) {
    ime = true;
    imeScheduled = false;
  }

  // do the interrupts
  handleInterrupts();

  // if halted, just chill for 4 cycles
  if (halted) {
    totalCycles += 4;
    return 4;
  }

  // fetch and execute that shit
  u8 opcode = fetchByte();
  u32 cycles = executeOpcode(opcode);
  totalCycles += cycles;
  return cycles;
}

u32 CPU::executeOpcode(u8 opcode) {
  u8 cycles = OPCODE_CYCLES[opcode];

  switch (opcode) {
  // 0x00 - 0x0F
  case 0x00:
    break; // NOP
  case 0x01:
    setBC(fetchWord());
    break; // LD BC,d16
  case 0x02:
    write8(getBC(), a);
    break; // LD (BC),A
  case 0x03:
    setBC(getBC() + 1);
    break; // INC BC
  case 0x04:
    b = inc8(b);
    break; // INC B
  case 0x05:
    b = dec8(b);
    break; // DEC B
  case 0x06:
    b = fetchByte();
    break;     // LD B,d8
  case 0x07: { // RLCA
    u8 carry = (a >> 7) & 1;
    a = (a << 1) | carry;
    setZ(false);
    setN(false);
    setH(false);
    setC(carry);
    break;
  }
  case 0x08:
    write16(fetchWord(), sp);
    break; // LD (a16),SP
  case 0x09:
    addHL(getBC());
    break; // ADD HL,BC
  case 0x0A:
    a = read8(getBC());
    break; // LD A,(BC)
  case 0x0B:
    setBC(getBC() - 1);
    break; // DEC BC
  case 0x0C:
    c = inc8(c);
    break; // INC C
  case 0x0D:
    c = dec8(c);
    break; // DEC C
  case 0x0E:
    c = fetchByte();
    break;     // LD C,d8
  case 0x0F: { // RRCA
    u8 carry = a & 1;
    a = (a >> 1) | (carry << 7);
    setZ(false);
    setN(false);
    setH(false);
    setC(carry);
    break;
  }

  // 0x10 - 0x1F
  case 0x10:
    stopped = true;
    fetchByte();
    break; // STOP
  case 0x11:
    setDE(fetchWord());
    break; // LD DE,d16
  case 0x12:
    write8(getDE(), a);
    break; // LD (DE),A
  case 0x13:
    setDE(getDE() + 1);
    break; // INC DE
  case 0x14:
    d = inc8(d);
    break; // INC D
  case 0x15:
    d = dec8(d);
    break; // DEC D
  case 0x16:
    d = fetchByte();
    break;     // LD D,d8
  case 0x17: { // RLA
    u8 oldCarry = getC() ? 1 : 0;
    u8 newCarry = (a >> 7) & 1;
    a = (a << 1) | oldCarry;
    setZ(false);
    setN(false);
    setH(false);
    setC(newCarry);
    break;
  }
  case 0x18:
    jr(true);
    break; // JR r8
  case 0x19:
    addHL(getDE());
    break; // ADD HL,DE
  case 0x1A:
    a = read8(getDE());
    break; // LD A,(DE)
  case 0x1B:
    setDE(getDE() - 1);
    break; // DEC DE
  case 0x1C:
    e = inc8(e);
    break; // INC E
  case 0x1D:
    e = dec8(e);
    break; // DEC E
  case 0x1E:
    e = fetchByte();
    break;     // LD E,d8
  case 0x1F: { // RRA
    u8 oldCarry = getC() ? 0x80 : 0;
    u8 newCarry = a & 1;
    a = (a >> 1) | oldCarry;
    setZ(false);
    setN(false);
    setH(false);
    setC(newCarry);
    break;
  }

  // 0x20 - 0x2F
  case 0x20:
    jr(!getZ());
    if (!getZ())
      cycles = CYCLES_JR_TAKEN;
    break; // JR NZ,r8
  case 0x21:
    setHL(fetchWord());
    break; // LD HL,d16
  case 0x22:
    write8(getHL(), a);
    setHL(getHL() + 1);
    break; // LD (HL+),A
  case 0x23:
    setHL(getHL() + 1);
    break; // INC HL
  case 0x24:
    h = inc8(h);
    break; // INC H
  case 0x25:
    h = dec8(h);
    break; // DEC H
  case 0x26:
    h = fetchByte();
    break;     // LD H,d8
  case 0x27: { // DAA
    u8 correction = 0;
    bool setCarry = false;
    if (getH() || (!getN() && (a & 0x0F) > 9)) {
      correction |= 0x06;
    }
    if (getC() || (!getN() && a > 0x99)) {
      correction |= 0x60;
      setCarry = true;
    }
    a += getN() ? -correction : correction;
    setZ(a == 0);
    setH(false);
    setC(setCarry);
    break;
  }
  case 0x28:
    jr(getZ());
    if (getZ())
      cycles = CYCLES_JR_TAKEN;
    break; // JR Z,r8
  case 0x29:
    addHL(getHL());
    break; // ADD HL,HL
  case 0x2A:
    a = read8(getHL());
    setHL(getHL() + 1);
    break; // LD A,(HL+)
  case 0x2B:
    setHL(getHL() - 1);
    break; // DEC HL
  case 0x2C:
    l = inc8(l);
    break; // INC L
  case 0x2D:
    l = dec8(l);
    break; // DEC L
  case 0x2E:
    l = fetchByte();
    break; // LD L,d8
  case 0x2F:
    a = ~a;
    setN(true);
    setH(true);
    break; // CPL

  // 0x30 - 0x3F
  case 0x30:
    jr(!getC());
    if (!getC())
      cycles = CYCLES_JR_TAKEN;
    break; // JR NC,r8
  case 0x31:
    sp = fetchWord();
    break; // LD SP,d16
  case 0x32:
    write8(getHL(), a);
    setHL(getHL() - 1);
    break; // LD (HL-),A
  case 0x33:
    sp++;
    break; // INC SP
  case 0x34:
    write8(getHL(), inc8(read8(getHL())));
    break; // INC (HL)
  case 0x35:
    write8(getHL(), dec8(read8(getHL())));
    break; // DEC (HL)
  case 0x36:
    write8(getHL(), fetchByte());
    break; // LD (HL),d8
  case 0x37:
    setN(false);
    setH(false);
    setC(true);
    break; // SCF
  case 0x38:
    jr(getC());
    if (getC())
      cycles = CYCLES_JR_TAKEN;
    break; // JR C,r8
  case 0x39:
    addHL(sp);
    break; // ADD HL,SP
  case 0x3A:
    a = read8(getHL());
    setHL(getHL() - 1);
    break; // LD A,(HL-)
  case 0x3B:
    sp--;
    break; // DEC SP
  case 0x3C:
    a = inc8(a);
    break; // INC A
  case 0x3D:
    a = dec8(a);
    break; // DEC A
  case 0x3E:
    a = fetchByte();
    break; // LD A,d8
  case 0x3F:
    setN(false);
    setH(false);
    setC(!getC());
    break; // CCF

  // 0x40 - 0x4F: LD B/C,r
  case 0x40:
    break; // LD B,B
  case 0x41:
    b = c;
    break;
  case 0x42:
    b = d;
    break;
  case 0x43:
    b = e;
    break;
  case 0x44:
    b = h;
    break;
  case 0x45:
    b = l;
    break;
  case 0x46:
    b = read8(getHL());
    break;
  case 0x47:
    b = a;
    break;
  case 0x48:
    c = b;
    break;
  case 0x49:
    break; // LD C,C
  case 0x4A:
    c = d;
    break;
  case 0x4B:
    c = e;
    break;
  case 0x4C:
    c = h;
    break;
  case 0x4D:
    c = l;
    break;
  case 0x4E:
    c = read8(getHL());
    break;
  case 0x4F:
    c = a;
    break;

  // 0x50 - 0x5F: LD D/E,r
  case 0x50:
    d = b;
    break;
  case 0x51:
    d = c;
    break;
  case 0x52:
    break; // LD D,D
  case 0x53:
    d = e;
    break;
  case 0x54:
    d = h;
    break;
  case 0x55:
    d = l;
    break;
  case 0x56:
    d = read8(getHL());
    break;
  case 0x57:
    d = a;
    break;
  case 0x58:
    e = b;
    break;
  case 0x59:
    e = c;
    break;
  case 0x5A:
    e = d;
    break;
  case 0x5B:
    break; // LD E,E
  case 0x5C:
    e = h;
    break;
  case 0x5D:
    e = l;
    break;
  case 0x5E:
    e = read8(getHL());
    break;
  case 0x5F:
    e = a;
    break;

  // 0x60 - 0x6F: LD H/L,r
  case 0x60:
    h = b;
    break;
  case 0x61:
    h = c;
    break;
  case 0x62:
    h = d;
    break;
  case 0x63:
    h = e;
    break;
  case 0x64:
    break; // LD H,H
  case 0x65:
    h = l;
    break;
  case 0x66:
    h = read8(getHL());
    break;
  case 0x67:
    h = a;
    break;
  case 0x68:
    l = b;
    break;
  case 0x69:
    l = c;
    break;
  case 0x6A:
    l = d;
    break;
  case 0x6B:
    l = e;
    break;
  case 0x6C:
    l = h;
    break;
  case 0x6D:
    break; // LD L,L
  case 0x6E:
    l = read8(getHL());
    break;
  case 0x6F:
    l = a;
    break;

  // 0x70 - 0x7F: LD (HL),r and LD A,r
  case 0x70:
    write8(getHL(), b);
    break;
  case 0x71:
    write8(getHL(), c);
    break;
  case 0x72:
    write8(getHL(), d);
    break;
  case 0x73:
    write8(getHL(), e);
    break;
  case 0x74:
    write8(getHL(), h);
    break;
  case 0x75:
    write8(getHL(), l);
    break;
  case 0x76:
    halted = true;
    break; // HALT
  case 0x77:
    write8(getHL(), a);
    break;
  case 0x78:
    a = b;
    break;
  case 0x79:
    a = c;
    break;
  case 0x7A:
    a = d;
    break;
  case 0x7B:
    a = e;
    break;
  case 0x7C:
    a = h;
    break;
  case 0x7D:
    a = l;
    break;
  case 0x7E:
    a = read8(getHL());
    break;
  case 0x7F:
    break; // LD A,A

  // 0x80 - 0x8F: ADD/ADC A,r
  case 0x80:
    add8(b);
    break;
  case 0x81:
    add8(c);
    break;
  case 0x82:
    add8(d);
    break;
  case 0x83:
    add8(e);
    break;
  case 0x84:
    add8(h);
    break;
  case 0x85:
    add8(l);
    break;
  case 0x86:
    add8(read8(getHL()));
    break;
  case 0x87:
    add8(a);
    break;
  case 0x88:
    adc8(b);
    break;
  case 0x89:
    adc8(c);
    break;
  case 0x8A:
    adc8(d);
    break;
  case 0x8B:
    adc8(e);
    break;
  case 0x8C:
    adc8(h);
    break;
  case 0x8D:
    adc8(l);
    break;
  case 0x8E:
    adc8(read8(getHL()));
    break;
  case 0x8F:
    adc8(a);
    break;

  // 0x90 - 0x9F: SUB/SBC A,r
  case 0x90:
    sub8(b);
    break;
  case 0x91:
    sub8(c);
    break;
  case 0x92:
    sub8(d);
    break;
  case 0x93:
    sub8(e);
    break;
  case 0x94:
    sub8(h);
    break;
  case 0x95:
    sub8(l);
    break;
  case 0x96:
    sub8(read8(getHL()));
    break;
  case 0x97:
    sub8(a);
    break;
  case 0x98:
    sbc8(b);
    break;
  case 0x99:
    sbc8(c);
    break;
  case 0x9A:
    sbc8(d);
    break;
  case 0x9B:
    sbc8(e);
    break;
  case 0x9C:
    sbc8(h);
    break;
  case 0x9D:
    sbc8(l);
    break;
  case 0x9E:
    sbc8(read8(getHL()));
    break;
  case 0x9F:
    sbc8(a);
    break;

  // 0xA0 - 0xAF: AND/XOR A,r
  case 0xA0:
    and8(b);
    break;
  case 0xA1:
    and8(c);
    break;
  case 0xA2:
    and8(d);
    break;
  case 0xA3:
    and8(e);
    break;
  case 0xA4:
    and8(h);
    break;
  case 0xA5:
    and8(l);
    break;
  case 0xA6:
    and8(read8(getHL()));
    break;
  case 0xA7:
    and8(a);
    break;
  case 0xA8:
    xor8(b);
    break;
  case 0xA9:
    xor8(c);
    break;
  case 0xAA:
    xor8(d);
    break;
  case 0xAB:
    xor8(e);
    break;
  case 0xAC:
    xor8(h);
    break;
  case 0xAD:
    xor8(l);
    break;
  case 0xAE:
    xor8(read8(getHL()));
    break;
  case 0xAF:
    xor8(a);
    break;

  // 0xB0 - 0xBF: OR/CP A,r
  case 0xB0:
    or8(b);
    break;
  case 0xB1:
    or8(c);
    break;
  case 0xB2:
    or8(d);
    break;
  case 0xB3:
    or8(e);
    break;
  case 0xB4:
    or8(h);
    break;
  case 0xB5:
    or8(l);
    break;
  case 0xB6:
    or8(read8(getHL()));
    break;
  case 0xB7:
    or8(a);
    break;
  case 0xB8:
    cp8(b);
    break;
  case 0xB9:
    cp8(c);
    break;
  case 0xBA:
    cp8(d);
    break;
  case 0xBB:
    cp8(e);
    break;
  case 0xBC:
    cp8(h);
    break;
  case 0xBD:
    cp8(l);
    break;
  case 0xBE:
    cp8(read8(getHL()));
    break;
  case 0xBF:
    cp8(a);
    break;

  // 0xC0 - 0xCF
  case 0xC0:
    ret(!getZ());
    if (!getZ())
      cycles = CYCLES_RET_TAKEN;
    break; // RET NZ
  case 0xC1:
    setBC(pop16());
    break; // POP BC
  case 0xC2:
    jp(!getZ());
    if (!getZ())
      cycles = CYCLES_JP_TAKEN;
    break; // JP NZ,a16
  case 0xC3:
    pc = fetchWord();
    break; // JP a16
  case 0xC4:
    call(!getZ());
    if (!getZ())
      cycles = CYCLES_CALL_TAKEN;
    break; // CALL NZ,a16
  case 0xC5:
    push16(getBC());
    break; // PUSH BC
  case 0xC6:
    add8(fetchByte());
    break; // ADD A,d8
  case 0xC7:
    rst(0x00);
    break; // RST 00H
  case 0xC8:
    ret(getZ());
    if (getZ())
      cycles = CYCLES_RET_TAKEN;
    break; // RET Z
  case 0xC9:
    pc = pop16();
    break; // RET
  case 0xCA:
    jp(getZ());
    if (getZ())
      cycles = CYCLES_JP_TAKEN;
    break; // JP Z,a16
  case 0xCB:
    cycles = executeCBOpcode(fetchByte());
    break; // CB prefix
  case 0xCC:
    call(getZ());
    if (getZ())
      cycles = CYCLES_CALL_TAKEN;
    break; // CALL Z,a16
  case 0xCD:
    call(true);
    break; // CALL a16
  case 0xCE:
    adc8(fetchByte());
    break; // ADC A,d8
  case 0xCF:
    rst(0x08);
    break; // RST 08H

  // 0xD0 - 0xDF
  case 0xD0:
    ret(!getC());
    if (!getC())
      cycles = CYCLES_RET_TAKEN;
    break; // RET NC
  case 0xD1:
    setDE(pop16());
    break; // POP DE
  case 0xD2:
    jp(!getC());
    if (!getC())
      cycles = CYCLES_JP_TAKEN;
    break; // JP NC,a16
  // 0xD3 unused
  case 0xD4:
    call(!getC());
    if (!getC())
      cycles = CYCLES_CALL_TAKEN;
    break; // CALL NC,a16
  case 0xD5:
    push16(getDE());
    break; // PUSH DE
  case 0xD6:
    sub8(fetchByte());
    break; // SUB d8
  case 0xD7:
    rst(0x10);
    break; // RST 10H
  case 0xD8:
    ret(getC());
    if (getC())
      cycles = CYCLES_RET_TAKEN;
    break; // RET C
  case 0xD9:
    pc = pop16();
    ime = true;
    break; // RETI
  case 0xDA:
    jp(getC());
    if (getC())
      cycles = CYCLES_JP_TAKEN;
    break; // JP C,a16
  // 0xDB unused
  case 0xDC:
    call(getC());
    if (getC())
      cycles = CYCLES_CALL_TAKEN;
    break; // CALL C,a16
  // 0xDD unused
  case 0xDE:
    sbc8(fetchByte());
    break; // SBC A,d8
  case 0xDF:
    rst(0x18);
    break; // RST 18H

  // 0xE0 - 0xEF
  case 0xE0:
    write8(0xFF00 + fetchByte(), a);
    break; // LDH (a8),A
  case 0xE1:
    setHL(pop16());
    break; // POP HL
  case 0xE2:
    write8(0xFF00 + c, a);
    break; // LD (C),A
  // 0xE3, 0xE4 unused
  case 0xE5:
    push16(getHL());
    break; // PUSH HL
  case 0xE6:
    and8(fetchByte());
    break; // AND d8
  case 0xE7:
    rst(0x20);
    break; // RST 20H
  case 0xE8:
    addSP(static_cast<s8>(fetchByte()));
    break; // ADD SP,r8
  case 0xE9:
    pc = getHL();
    break; // JP HL
  case 0xEA:
    write8(fetchWord(), a);
    break; // LD (a16),A
  // 0xEB, 0xEC, 0xED unused
  case 0xEE:
    xor8(fetchByte());
    break; // XOR d8
  case 0xEF:
    rst(0x28);
    break; // RST 28H

  // 0xF0 - 0xFF
  case 0xF0:
    a = read8(0xFF00 + fetchByte());
    break; // LDH A,(a8)
  case 0xF1:
    setAF(pop16());
    break; // POP AF
  case 0xF2:
    a = read8(0xFF00 + c);
    break; // LD A,(C)
  case 0xF3:
    ime = false;
    break; // DI
  // 0xF4 unused
  case 0xF5:
    push16(getAF());
    break; // PUSH AF
  case 0xF6:
    or8(fetchByte());
    break; // OR d8
  case 0xF7:
    rst(0x30);
    break;     // RST 30H
  case 0xF8: { // LD HL,SP+r8
    s8 offset = static_cast<s8>(fetchByte());
    setHL(sp + offset);
    setZ(false);
    setN(false);
    setH(((sp & 0x0F) + (offset & 0x0F)) > 0x0F);
    setC(((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
    break;
  }
  case 0xF9:
    sp = getHL();
    break; // LD SP,HL
  case 0xFA:
    a = read8(fetchWord());
    break; // LD A,(a16)
  case 0xFB:
    imeScheduled = true;
    break; // EI
  // 0xFC, 0xFD unused
  case 0xFE:
    cp8(fetchByte());
    break; // CP d8
  case 0xFF:
    rst(0x38);
    break; // RST 38H

  default:
    // Undefined opcode - treat as NOP
    break;
  }

  return cycles;
}

u32 CPU::executeCBOpcode(u8 opcode) {
  u8 cycles = CB_OPCODE_CYCLES[opcode];

  // Get source/destination register
  u8 *reg = nullptr;
  u8 memVal = 0;
  bool isMem = false;

  switch (opcode & 0x07) {
  case 0:
    reg = &b;
    break;
  case 1:
    reg = &c;
    break;
  case 2:
    reg = &d;
    break;
  case 3:
    reg = &e;
    break;
  case 4:
    reg = &h;
    break;
  case 5:
    reg = &l;
    break;
  case 6:
    isMem = true;
    memVal = read8(getHL());
    break;
  case 7:
    reg = &a;
    break;
  }

  u8 val = isMem ? memVal : *reg;
  u8 result = val;

  switch (opcode & 0xF8) {
  // Rotates and shifts
  case 0x00:
    result = rlc(val);
    break; // RLC
  case 0x08:
    result = rrc(val);
    break; // RRC
  case 0x10:
    result = rl(val);
    break; // RL
  case 0x18:
    result = rr(val);
    break; // RR
  case 0x20:
    result = sla(val);
    break; // SLA
  case 0x28:
    result = sra(val);
    break; // SRA
  case 0x30:
    result = swap(val);
    break; // SWAP
  case 0x38:
    result = srl(val);
    break; // SRL

  // BIT (test bit)
  case 0x40:
  case 0x48:
  case 0x50:
  case 0x58:
  case 0x60:
  case 0x68:
  case 0x70:
  case 0x78:
    bit((opcode >> 3) & 0x07, val);
    return cycles; // BIT doesn't modify the value

  // RES (reset bit)
  case 0x80:
  case 0x88:
  case 0x90:
  case 0x98:
  case 0xA0:
  case 0xA8:
  case 0xB0:
  case 0xB8:
    result = res((opcode >> 3) & 0x07, val);
    break;

  // SET (set bit)
  case 0xC0:
  case 0xC8:
  case 0xD0:
  case 0xD8:
  case 0xE0:
  case 0xE8:
  case 0xF0:
  case 0xF8:
    result = set((opcode >> 3) & 0x07, val);
    break;
  }

  // Write result back
  if (isMem) {
    write8(getHL(), result);
  } else {
    *reg = result;
  }

  return cycles;
}

} // namespace jester
