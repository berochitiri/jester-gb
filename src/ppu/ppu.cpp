#include "ppu/ppu.hpp"

namespace jester {

PPU::PPU() { reset(); }

void PPU::reset() {
  vram.fill(0);
  oam.fill(0);
  frameBuffer.fill(0);

  // ppu is awake now mfs (dmg state)
  lcdc = 0x91; // LCD on, BG enabled
  stat = 0x85;
  scy = 0;
  scx = 0;
  ly = 0;
  lyc = 0;
  bgp = 0xFC;
  obp0 = 0xFF;
  obp1 = 0xFF;
  wy = 0;
  wx = 0;

  modeClock = 0;
  mode = MODE_OAM;
  frameReady = false;
  windowLine = 0;

  vblankInterrupt = false;
  statInterrupt = false;
}

void PPU::step(u32 cycles) {
  // screen is off lol (lcd disabled)
  if (!(lcdc & 0x80)) {
    return;
  }

  modeClock += cycles;

  switch (mode) {
  case MODE_OAM: // searchin oam for sprites
    if (modeClock >= CYCLES_OAM) {
      modeClock -= CYCLES_OAM;
      setMode(MODE_VRAM);
    }
    break;

  case MODE_VRAM: // reading vram data shit
    if (modeClock >= CYCLES_VRAM) {
      modeClock -= CYCLES_VRAM;

      // render the actual line
      renderScanline();

      setMode(MODE_HBLANK);
    }
    break;

  case MODE_HBLANK: // h-blank chill time
    if (modeClock >= CYCLES_HBLANK) {
      modeClock -= CYCLES_HBLANK;
      ly++;
      checkLYC();

      if (ly >= LINES_VISIBLE) {
        // vblank time baby
        setMode(MODE_VBLANK);
        frameReady = true;
        vblankInterrupt = true;
      } else {
        setMode(MODE_OAM);
      }
    }
    break;

  case MODE_VBLANK: // v-blank chill time
    if (modeClock >= CYCLES_LINE) {
      modeClock -= CYCLES_LINE;
      ly++;

      if (ly >= LINES_TOTAL) {
        ly = 0;
        windowLine = 0;
        setMode(MODE_OAM);
      }
      checkLYC();
    }
    break;
  }
}

void PPU::setMode(u8 newMode) {
  mode = newMode;
  stat = (stat & 0xFC) | mode;

  // check if any stat interrupts are screaming
  bool interrupt = false;
  if ((newMode == MODE_HBLANK) && (stat & 0x08))
    interrupt = true;
  if ((newMode == MODE_VBLANK) && (stat & 0x10))
    interrupt = true;
  if ((newMode == MODE_OAM) && (stat & 0x20))
    interrupt = true;

  if (interrupt) {
    statInterrupt = true;
  }
}

void PPU::checkLYC() {
  if (ly == lyc) {
    stat |= 0x04; // found a match (coincidence flag)
    if (stat & 0x40) {
      statInterrupt = true;
    }
  } else {
    stat &= ~0x04;
  }
}

void PPU::renderScanline() {
  if (ly >= SCREEN_HEIGHT)
    return;

  // clear the line junk
  for (u16 x = 0; x < SCREEN_WIDTH; x++) {
    frameBuffer[ly * SCREEN_WIDTH + x] = 0;
  }

  // draw the layers
  if (lcdc & 0x01) { // bg is on
    renderBackground(ly);
  }

  if ((lcdc & 0x20) && (ly >= wy)) { // window is on and visible
    renderWindow(ly);
  }

  if (lcdc & 0x02) { // sprites are on
    renderSprites(ly);
  }
}

void PPU::renderBackground(u8 scanline) {
  // Tile map base
  u16 tileMapBase = (lcdc & 0x08) ? 0x1C00 : 0x1800;
  // Tile data base
  bool signedAddr = !(lcdc & 0x10);
  u16 tileDataBase = signedAddr ? 0x0800 : 0x0000;

  u8 y = scanline + scy;
  u8 tileRow = y / 8;
  u8 pixelY = y % 8;

  for (u16 x = 0; x < SCREEN_WIDTH; x++) {
    u8 scrolledX = x + scx;
    u8 tileCol = scrolledX / 8;
    u8 pixelX = scrolledX % 8;

    // Get tile index
    u16 tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
    u8 tileIndex = vram[tileMapAddr];

    // Get tile data address
    u16 tileAddr;
    if (signedAddr) {
      s8 signedIndex = static_cast<s8>(tileIndex);
      tileAddr = tileDataBase + ((signedIndex + 128) * 16);
    } else {
      tileAddr = tileDataBase + (tileIndex * 16);
    }

    // Get pixel color from tile
    u16 rowAddr = tileAddr + (pixelY * 2);
    u8 lo = vram[rowAddr];
    u8 hi = vram[rowAddr + 1];

    u8 bit = 7 - pixelX;
    u8 colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
    u8 color = getColorFromPalette(colorNum, bgp);

    frameBuffer[scanline * SCREEN_WIDTH + x] = color;
  }
}

void PPU::renderWindow(u8 scanline) {
  if (wx > 166 || wy > scanline)
    return;

  // Tile map base for window
  u16 tileMapBase = (lcdc & 0x40) ? 0x1C00 : 0x1800;
  bool signedAddr = !(lcdc & 0x10);
  u16 tileDataBase = signedAddr ? 0x0800 : 0x0000;

  u8 y = windowLine;
  u8 tileRow = y / 8;
  u8 pixelY = y % 8;

  s16 windowX = wx - 7;
  bool drewWindow = false;

  for (u16 x = 0; x < SCREEN_WIDTH; x++) {
    if (static_cast<s16>(x) < windowX)
      continue;
    drewWindow = true;

    u8 windowPixelX = x - windowX;
    u8 tileCol = windowPixelX / 8;
    u8 pixelX = windowPixelX % 8;

    u16 tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
    u8 tileIndex = vram[tileMapAddr];

    u16 tileAddr;
    if (signedAddr) {
      s8 signedIndex = static_cast<s8>(tileIndex);
      tileAddr = tileDataBase + ((signedIndex + 128) * 16);
    } else {
      tileAddr = tileDataBase + (tileIndex * 16);
    }

    u16 rowAddr = tileAddr + (pixelY * 2);
    u8 lo = vram[rowAddr];
    u8 hi = vram[rowAddr + 1];

    u8 bit = 7 - pixelX;
    u8 colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
    u8 color = getColorFromPalette(colorNum, bgp);

    frameBuffer[scanline * SCREEN_WIDTH + x] = color;
  }

  if (drewWindow) {
    windowLine++;
  }
}

void PPU::renderSprites(u8 scanline) {
  u8 spriteHeight = (lcdc & 0x04) ? 16 : 8; // big or small bois?

  // find sprites for this line (max 10 allowed by hardware)
  struct SpriteEntry {
    u8 x, y, tile, flags;
    u8 oamIndex;
  };
  SpriteEntry sprites[10];
  u8 spriteCount = 0;

  for (u8 i = 0; i < 40 && spriteCount < 10; i++) {
    u8 y = oam[i * 4] - 16;
    u8 x = oam[i * 4 + 1] - 8;

    if (scanline >= y && scanline < y + spriteHeight) {
      sprites[spriteCount].y = y;
      sprites[spriteCount].x = x;
      sprites[spriteCount].tile = oam[i * 4 + 2];
      sprites[spriteCount].flags = oam[i * 4 + 3];
      sprites[spriteCount].oamIndex = i;
      spriteCount++;
    }
  }

  // sort by X (leftmost sprites win)
  // In case of tie, lower OAM index wins
  for (u8 i = 0; i < spriteCount; i++) {
    for (u8 j = i + 1; j < spriteCount; j++) {
      if (sprites[j].x < sprites[i].x ||
          (sprites[j].x == sprites[i].x &&
           sprites[j].oamIndex < sprites[i].oamIndex)) {
        SpriteEntry temp = sprites[i];
        sprites[i] = sprites[j];
        sprites[j] = temp;
      }
    }
  }

  // draw sprites (backwards so correct ones are on top)
  for (s8 i = spriteCount - 1; i >= 0; i--) {
    SpriteEntry &spr = sprites[i];

    bool flipX = spr.flags & 0x20;
    bool flipY = spr.flags & 0x40;
    bool priority = spr.flags & 0x80; // 0=above BG, 1=behind BG colors 1-3
    u8 palette = (spr.flags & 0x10) ? obp1 : obp0;

    u8 tileY = scanline - spr.y;
    if (flipY) {
      tileY = spriteHeight - 1 - tileY;
    }

    u8 tileIndex = spr.tile;
    if (spriteHeight == 16) {
      tileIndex &= 0xFE; // Clear bit 0 for 8x16 mode
      if (tileY >= 8) {
        tileIndex++;
        tileY -= 8;
      }
    }

    u16 tileAddr = tileIndex * 16 + tileY * 2;
    u8 lo = vram[tileAddr];
    u8 hi = vram[tileAddr + 1];

    for (u8 px = 0; px < 8; px++) {
      s16 screenX = spr.x + px;
      if (screenX < 0 || screenX >= SCREEN_WIDTH)
        continue;

      u8 bit = flipX ? px : (7 - px);
      u8 colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);

      // color 0 is invisible for sprites (obv)
      if (colorNum == 0)
        continue;

      // Check priority
      if (priority && frameBuffer[scanline * SCREEN_WIDTH + screenX] != 0) {
        continue;
      }

      u8 color = getColorFromPalette(colorNum, palette);
      frameBuffer[scanline * SCREEN_WIDTH + screenX] = color;
    }
  }
}

u8 PPU::getColorFromPalette(u8 colorNum, u8 palette) const {
  return (palette >> (colorNum * 2)) & 0x03;
}

u8 PPU::readVRAM(u16 addr) const {
  if (addr < vram.size()) {
    return vram[addr];
  }
  return 0xFF;
}

void PPU::writeVRAM(u16 addr, u8 val) {
  if (addr < vram.size()) {
    vram[addr] = val;
  }
}

u8 PPU::readOAM(u16 addr) const {
  if (addr < oam.size()) {
    return oam[addr];
  }
  return 0xFF;
}

void PPU::writeOAM(u16 addr, u8 val) {
  if (addr < oam.size()) {
    oam[addr] = val;
  }
}

u8 PPU::readRegister(u16 addr) const {
  switch (addr) {
  case LCDC:
    return lcdc;
  case STAT:
    return stat | 0x80; // Bit 7 always 1
  case SCY:
    return scy;
  case SCX:
    return scx;
  case LY:
    return ly;
  case LYC:
    return lyc;
  case BGP:
    return bgp;
  case OBP0:
    return obp0;
  case OBP1:
    return obp1;
  case WY:
    return wy;
  case WX:
    return wx;
  default:
    return 0xFF;
  }
}

void PPU::writeRegister(u16 addr, u8 val) {
  switch (addr) {
  case LCDC:
    // Turning LCD off resets LY
    if ((lcdc & 0x80) && !(val & 0x80)) {
      ly = 0;
      modeClock = 0;
      mode = MODE_HBLANK;
      stat = (stat & 0xFC) | mode;
    }
    lcdc = val;
    break;
  case STAT:
    stat = (stat & 0x87) | (val & 0x78);
    break; // Bits 0-2 read-only
  case SCY:
    scy = val;
    break;
  case SCX:
    scx = val;
    break;
  case LYC:
    lyc = val;
    checkLYC();
    break;
  case BGP:
    bgp = val;
    break;
  case OBP0:
    obp0 = val;
    break;
  case OBP1:
    obp1 = val;
    break;
  case WY:
    wy = val;
    break;
  case WX:
    wx = val;
    break;
  }
}

} // namespace jester
