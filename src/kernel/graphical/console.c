#include <console.h>
#include <fs_controller.h>
#include <system.h>
#define _STDINT_H
#define SSFN_CONSOLEBITMAP_CONTROL
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn.h>

// Kernel console implementation
// Copyright (C) 2023 Panagiotis

int bg_color[] = {0, 0, 0};
int textcolor[] = {255, 255, 255};

uint32_t rgbToHex(int r, int g, int b) {
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

uint32_t rgbaToHex(int r, int g, int b, int a) {
  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) +
         (a & 0xff);
}

void initiateConsole() {
  drawClearScreen();

  ssfn_dst.ptr = framebuffer;     /* address of the linear frame buffer */
  ssfn_dst.w = framebufferWidth;  /* width */
  ssfn_dst.h = framebufferHeight; /* height */
  ssfn_dst.p = framebufferPitch;  /* bytes per line */
  ssfn_dst.x = ssfn_dst.y = 0;    /* pen position */
  ssfn_dst.fg =
      rgbToHex(textcolor[0], textcolor[1], textcolor[2]); /* foreground color */

  OpenFile *dir = fsKernelOpen(DEFAULT_FONT_PATH);
  if (!dir) {
    debugf("[console] Could not open default font file: %s!\n",
           DEFAULT_FONT_PATH);
    printf("Cannot open VGA font!\n");
    return;
  }
  uint32_t filesize = fsGetFilesize(dir);
  uint8_t *buff = (uint8_t *)malloc(filesize);
  fsReadFullFile(dir, buff);

  ssfn_src = buff;
  fsKernelClose(dir);
  debugf("[console] Initiated with font: filepath{%s} filesize{%d}\n",
         DEFAULT_FONT_PATH, filesize);
}

void drawClearScreen() {
  ssfn_dst.x = 0;
  ssfn_dst.y = 0;
  drawRect(0, 0, framebufferWidth, framebufferHeight, bg_color[0], bg_color[1],
           bg_color[2]);
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

void drawCharacter(int charnum) {
  if ((ssfn_dst.y + TTY_CHARACTER_HEIGHT) >= framebufferHeight) {
    // clearScreen();
  }

  if (charnum == 8) {
    ssfn_dst.x -= TTY_CHARACTER_WIDTH;
    drawRect(ssfn_dst.x, ssfn_dst.y, TTY_CHARACTER_WIDTH, TTY_CHARACTER_HEIGHT,
             bg_color[0], bg_color[1], bg_color[2]);
    return;
  }

  // debugf("[console::ptr] Before drawing: x{%d} y{%d}\n", ssfn_dst.x,
  //        ssfn_dst.y);
  ssfn_putc(charnum); // 8x16 (x=8, y=16)
  // debugf("[console::ptr] After drawing: x{%d} y{%d}\n", ssfn_dst.x,
  //        ssfn_dst.y);
}
