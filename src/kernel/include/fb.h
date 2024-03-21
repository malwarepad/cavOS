#ifndef FB_H
#define FB_H

#include "types.h"

extern uint8_t *framebuffer;

extern uint16_t framebufferWidth;
extern uint16_t framebufferHeight;
extern uint32_t framebufferPitch;

void drawRect(int x, int y, int w, int h, int r, int g, int b);
void drawPixel(int x, int y, int r, int g, int b);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);
void drawCircle(int centerX, int centerY, int radius, int r, int g, int b);

#endif