#include <fb.h>
#include <malloc.h>
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
