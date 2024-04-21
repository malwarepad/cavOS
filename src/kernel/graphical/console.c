#include <console.h>
#include <fb.h>
#include <fs_controller.h>
#include <malloc.h>
#include <md5.h>
#include <psf.h>
#include <string.h>
#include <system.h>
#include <util.h>

// Kernel console implementation
// Copyright (C) 2024 Panagiotis

int bg_color[] = {0, 0, 0};
int textcolor[] = {255, 255, 255};

uint32_t width = 0;
uint32_t height = 0;

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
  if (check && !(height >= (framebufferHeight - CHAR_HEIGHT)))
    return false;

  for (int y = CHAR_HEIGHT; y < framebufferHeight; y++) {
    {
      void       *dest = (void *)(((size_t)framebuffer) +
                            (y - CHAR_HEIGHT) * framebufferPitch);
      const void *src = (void *)(((size_t)framebuffer) + y * framebufferPitch);
      memcpy(dest, src, framebufferWidth * 4);
    }
  }
  drawRect(0, framebufferHeight - CHAR_HEIGHT, framebufferWidth, CHAR_HEIGHT,
           bg_color[0], bg_color[1], bg_color[2]);
  height -= CHAR_HEIGHT;

  return true;
}

void eraseBull() {
  drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, bg_color[0], bg_color[1],
           bg_color[2]);
}

void updateBull() {
  if (width >= framebufferWidth) {
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
  drawRect(0, 0, framebufferWidth, framebufferHeight, bg_color[0], bg_color[1],
           bg_color[2]);
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
  if (!charnum)
    return;

  if (width > (framebufferWidth - CHAR_WIDTH)) {
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
    drawCharacter(-1);
    eraseBull();
    width = 0;
    height += CHAR_HEIGHT;
    break;
  case '\b':
    eraseBull();
    width -= CHAR_WIDTH;
    drawRect(width, height, CHAR_WIDTH, CHAR_HEIGHT, bg_color[0], bg_color[1],
             bg_color[2]);
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
  drawCharacter(character);
}

// printf.c uses this
void putchar_(char c) { printfch(c); }
