#ifndef VGA_H
#define VGA_H

#include "multiboot2.h"
#include "string.h"
#include "system.h"

void drawRect(int x, int y, int w, int h, int r, int g, int b);
void drawCharacter(int charnum);
void drawPixel(int x, int y, int r, int g, int b);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);
void changeBg(int r, int g, int b);
void changeTextColor(int r, int g, int b);
void drawClearScreen();
void drawCircle(int centerX, int centerY, int radius, int r, int g, int b);

#endif