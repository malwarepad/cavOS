#include <fb.h>
#include <malloc.h>
#include <paging.h>
#include <stdarg.h>
#include <system.h>
#include <util.h>

// System framebuffer manager
// Copyright (C) 2024 Panagiotis

uint8_t *framebuffer = 0;

uint16_t framebufferWidth;
uint16_t framebufferHeight;
uint32_t framebufferPitch;

void drawRect(int x, int y, int w, int h, int r, int g,
              int b) { // Draw a filled rectangle
  int            i, j;
  unsigned char *video = (unsigned char *)framebuffer;
  unsigned int   offset =
      (x + y * framebufferWidth) *
      4; // Finding the location of the pixel in the video array
  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) { // color each line
      video[offset + j * 4] = b;
      video[offset + j * 4 + 1] = g;
      video[offset + j * 4 + 2] = r;
      video[offset + j * 4 + 3] = 0;
    }
    offset += framebufferPitch; // switch to the beginnering of next line
  }
}

void drawPixel(int x, int y, int r, int g,
               int b) { // Set a specific pixel on screen to white
  unsigned char *video = (unsigned char *)framebuffer;

  unsigned int offset =
      (x + y * framebufferWidth) *
      4; // Finding the location of the pixel in the video array

  video[offset] = b; // Setting the color using BGR
  video[offset + 1] = g;
  video[offset + 2] = r;
  video[offset + 3] = 0;

  return;
}

void drawCircle(int centerX, int centerY, int radius, int r, int g, int b) {
  unsigned char *video = (unsigned char *)framebuffer;

  for (int y = centerY - radius; y <= centerY + radius; y++) {
    for (int x = centerX - radius; x <= centerX + radius; x++) {
      if ((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY) <=
          radius * radius) {
        unsigned int offset = (x + y * framebufferWidth) * 4;
        video[offset] = b;
        video[offset + 1] = g;
        video[offset + 2] = r;
        video[offset + 3] = 0;
      }
    }
  }
}

void drawLine(int x1, int y1, int x2, int y2, int r, int g,
              int b) { // Draw a line on the screen

  int dx = x2 - x1; // the horizontal distance of the line
  int dy = y2 - y1; // the vertical distance of the line
  int dyabs = dy;   // the absolute value of the two distances
  int dxabs = dx;
  int px = 0; // x-coordinate of location of the pixel
  int py = 0; // y-coordinate of location of the pixel
  int temp;   // used in comparison

  // Find the absolute values of distances
  if (dx < 0)
    dxabs = -dx;

  if (dy < 0)
    dyabs = -dy;

  if (dxabs >= dyabs) { // If the line is more horizontal than vertical */

    if (x1 > x2) { // Convering so that x1 is always the smaller value between
                   // x1 and x2
      temp = x2;
      x2 = x1;
      x1 = temp;
    }

    for (px = x1; px <= x2; px++) { // Draw line
      int tempy = dy * (px - x1);
      py = y1 + tempy / dxabs;
      drawPixel(px, py, r, g, b);
    }
  }

  else { /* the line is more vertical than horizontal */

    if (y1 > y2) { // Convering so that y1 is always the smaller value between
                   // y1 and y2
      temp = y2;
      y2 = y1;
      y1 = temp;
    }

    for (py = y1; py <= y2; py++) { // Draw line
      int tempx = dx * (py - y1);
      px = x1 + tempx / dyabs;
      drawPixel(px, py, r, g, b);
    }
  }
}

int fbUserIllegal() {
  debugf("[io::fb] Tried to do anything but an mmap/ioctl!\n");
  return -1;
}

int fbUserIoctl(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  case FBIOGET_VSCREENINFO: {
    struct fb_var_screeninfo *fb = arg;
    fb->xres = framebufferWidth;
    fb->yres = framebufferHeight;

    fb->xres_virtual = framebufferWidth;
    fb->yres_virtual = framebufferHeight;

    fb->bits_per_pixel = 0;
    fb->grayscale = 0;
    // fb->red = 0;
    // fb->green = 0;
    // fb->blue = 0;
    fb->nonstd = 0;
    fb->activate = 0;                   // idek
    fb->height = framebufferHeight / 4; // VERY approximate
    fb->width = framebufferWidth / 4;   // VERY approximate
    return 0;
    break;
  }
  default:
    return -1;
    break;
  }
}

size_t fbUserMmap(size_t addr, size_t length, int prot, int flags, OpenFile *fd,
                  size_t pgoffset) {
  size_t targPages = DivRoundUp(length, PAGE_SIZE);
  size_t physStart = VirtualToPhysical((size_t)framebuffer);
  for (int i = 0; i < targPages; i++) {
    VirtualMap(0x100000000000 + i * PAGE_SIZE, physStart + i * PAGE_SIZE,
               PF_RW | PF_USER);
  } // todo: get rid of hardcoded location!
  return 0x100000000000;
}

SpecialHandlers fb0 = {.read = fbUserIllegal,
                       .write = fbUserIllegal,
                       .ioctl = fbUserIoctl,
                       .mmap = fbUserMmap};
