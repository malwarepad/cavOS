#include <ansi.h>
#include <console.h>
#include <fb.h>
#include <malloc.h>
#include <md5.h>
#include <psf.h>
#include <string.h>
#include <system.h>
#include <util.h>
#include <vfs.h>

// Kernel console implementation
// Copyright (C) 2024 Panagiotis

Spinlock LOCK_CONSOLE = ATOMIC_FLAG_INIT;

int bg_color[] = {0, 0, 0};
int textcolor[] = {255, 255, 255};

uint32_t width = 0;
uint32_t height = 0;

atomic_bool consoleDisabled = false;

#define CHAR_HEIGHT (psf->height)
#define CHAR_WIDTH (8)

uint32_t rgbToHex(int r, int g, int b) {
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

uint32_t rgbaToHex(int r, int g, int b, int a) {
  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) +
         (a & 0xff);
}

void initiateConsole() {
  width = 0;
  height = 0;

  psfLoadDefaults();
}

bool scrollConsole(bool check) {
  if (check && !(height >= (fb.height - CHAR_HEIGHT)))
    return false;

  for (int y = CHAR_HEIGHT; y < fb.height; y++) {
    {
      void *dest = (void *)(((size_t)fb.virt) + (y - CHAR_HEIGHT) * fb.pitch);
      const void *src = (void *)(((size_t)fb.virt) + y * fb.pitch);
      memcpy(dest, src, fb.width * 4);
    }
  }
  drawRect(0, fb.height - CHAR_HEIGHT, fb.width, CHAR_HEIGHT, bg_color[0],
           bg_color[1], bg_color[2]);
  height -= CHAR_HEIGHT;

  return true;
}

bool cursorHidden = false;

void eraseBull() {
  if (cursorHidden)
    return;
  drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, bg_color[0], bg_color[1],
           bg_color[2]);
}

void updateBull() {
  if (cursorHidden)
    return;
  if (width >= fb.width) {
    bool neededScrolling = scrollConsole(true);
    if (!neededScrolling)
      height += CHAR_HEIGHT;
    width = 0;
  }
  drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, textcolor[0], textcolor[1],
           textcolor[2]);
}

void clearScreen() {
  width = 0;
  height = 0;
  drawRect(0, 0, fb.width, fb.height, bg_color[0], bg_color[1], bg_color[2]);
  updateBull();
}

void changeTextColor(int r, int g, int b) {
  textcolor[0] = r;
  textcolor[1] = g;
  textcolor[2] = b;
}

void changeBg(int r, int g, int b) {
  bg_color[0] = r;
  bg_color[1] = g;
  bg_color[2] = b;
}

void changeColor(int color[]) {
  textcolor[0] = color[0];
  textcolor[1] = color[1];
  textcolor[2] = color[2];
}

uint32_t getConsoleX() { return width; }
uint32_t getConsoleY() { return height; }

void setConsoleX(uint32_t x) {
  eraseBull();
  width = x;
  updateBull();
}
void setConsoleY(uint32_t y) {
  eraseBull();
  height = y;
  updateBull();
}

void drawCharacter(int charnum) {
  if (!charnum || consoleDisabled)
    return;

  if (ansiHandle(charnum))
    return;

  if (width > (fb.width - CHAR_WIDTH)) {
    width = 0;
    height += CHAR_HEIGHT;
  }

  scrollConsole(true);

  switch (charnum) {
  case -1:
    drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, bg_color[0], bg_color[1],
             bg_color[2]);
    width += CHAR_WIDTH;
    break;
  case '\n':
    // drawCharacter(-1);
    eraseBull();
    width = 0;
    height += CHAR_HEIGHT;
    break;
  case 0xd:
    eraseBull();
    width = 0;
    break;
  case 0xf: // todo: alternative character sets
    break;
  case '\b':
    eraseBull();
    width -= CHAR_WIDTH;
    drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, bg_color[0], bg_color[1],
             bg_color[2]);
    break;
  case '\t':
    for (int i = 0; i < 4; i++)
      drawCharacter(' ');
    break;
  default:
    eraseBull();
    psfPutC(charnum, width, height, textcolor[0], textcolor[1], textcolor[2]);
    width += CHAR_WIDTH;
    break;
  }
  updateBull();
}

void printfch(char character) {
  // debugf("%c", character);
  spinlockAcquire(&LOCK_CONSOLE);
  drawCharacter(character);
  spinlockRelease(&LOCK_CONSOLE);
}

// printf.c uses this
void putchar_(char c) { printfch(c); }
