#include "tui/renderer.hpp"
#include <cstdio>

namespace jester {

Renderer::Renderer() { brailleBuffer.fill(0); }

void Renderer::init(Terminal *term) { terminal = term; }

void Renderer::render(
    const std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT> &frameBuffer) {
  if (!terminal)
    return;

  // pack all this shit into one string cuz syscalls are expensive af
  std::string frame;
  frame.reserve(TERM_WIDTH * TERM_HEIGHT * 8); // pre-allocate memory mfs

  // pick a color and stick with it zoomers (speed hacks)
  frame += getColorCode(0); // Use brightest color always

  // Process the frame buffer in 2x4 blocks
  for (u16 by = 0; by < TERM_HEIGHT; by++) {
    // teleport the cursor to the right spot for this row
    char buf[16];
    snprintf(buf, sizeof(buf), "\033[%d;%dH", BORDER_Y + by + 1, BORDER_X + 1);
    frame += buf;

    for (u16 bx = 0; bx < TERM_WIDTH; bx++) {
      frame += renderBlock(frameBuffer, bx, by);
    }
  }

  terminal->write(frame);
  terminal->flush();
}

std::string
Renderer::renderBlock(const std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT> &fb,
                      u16 blockX, u16 blockY) {
  u16 px = blockX * 2;
  u16 py = blockY * 4;

  bool dots[8];
  bool hasContent = false;

  for (u8 row = 0; row < 4; row++) {
    for (u8 col = 0; col < 2; col++) {
      u16 x = px + col;
      u16 y = py + row;

      u8 color = 0;
      if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        color = fb[y * SCREEN_WIDTH + x];
      }

      // if the pixel is bright, give it a dot lol
      bool isLit = (color <= 1);
      dots[row + col * 4] = isLit;
      if (isLit)
        hasContent = true;
    }
  }

  if (!hasContent) {
    return " ";
  }

  return Terminal::brailleToUTF8(Terminal::pixelsToBraille(dots));
}

std::string Renderer::getColorCode(u8 gbColor) {
  (void)gbColor; // ignore this for now, single color is faster anyway

  switch (colorPalette) {
  case 0: // Pure white
    return "\033[38;2;255;255;255m";

  case 1: // Green DMG
    return "\033[38;2;155;188;15m";

  case 2: // Amber
    return "\033[38;2;255;176;0m";

  case 3: // Blue
    return "\033[38;2;200;220;255m";

  case 4: // Pink
    return "\033[38;2;255;100;255m";

  default:
    return "\033[38;2;0;255;0m";
  }
}

void Renderer::drawBorder() {
  if (!terminal)
    return;

  std::string border;
  border.reserve(1024);

  // edgy dark grey border
  border += "\033[38;2;60;60;60m";

  // top bar with the title mfs
  border += "\033[1;1H┌─ \033[38;2;100;255;100mJESTER-GB\033[38;2;60;60;60m ";
  for (u16 i = 0; i < TERM_WIDTH - 11; i++) {
    border += "─";
  }
  border += "┐";

  // side bars
  for (u16 y = 0; y < TERM_HEIGHT; y++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%d;1H│", BORDER_Y + y + 1);
    border += buf;
    snprintf(buf, sizeof(buf), "\033[%d;%dH│", BORDER_Y + y + 1,
             BORDER_X + TERM_WIDTH + 1);
    border += buf;
  }

  // Bottom border
  char buf[32];
  snprintf(buf, sizeof(buf), "\033[%d;1H└", BORDER_Y + TERM_HEIGHT + 1);
  border += buf;
  for (u16 i = 0; i < TERM_WIDTH; i++) {
    border += "─";
  }
  border += "┘";

  border += "\033[0m";
  terminal->write(border);
  terminal->flush();
}

void Renderer::renderDebug(u16 pc, u8 a, u8 f, u16 sp, double fps, u64 cycles) {
  if (!terminal)
    return;

  char buf[256];
  u16 debugY = BORDER_Y + TERM_HEIGHT + 2;

  snprintf(buf, sizeof(buf),
           "\033[%d;%dH\033[38;2;100;100;100mFPS:%.0f PC:%04X SP:%04X A:%02X "
           "[%c%c%c%c]",
           debugY, BORDER_X + 1, fps, pc, sp, a, (f & 0x80) ? 'Z' : '-',
           (f & 0x40) ? 'N' : '-', (f & 0x20) ? 'H' : '-',
           (f & 0x10) ? 'C' : '-');

  terminal->write(buf);

  (void)cycles; // dont show this, too much clutter on screen lol
}

void Renderer::setPalette(u8 palette) { colorPalette = palette; }

} // namespace jester
