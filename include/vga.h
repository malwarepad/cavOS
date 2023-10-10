#ifndef VGA_H
#define VGA_H

#include "system.h"
#include "string.h"
#include "multiboot2.h"

void drawRect(multiboot_info_t *mbi, int x, int y, int w, int h, int r, int g, int b);
void drawText(multiboot_info_t *mbi, int charnum);
void drawPixel(multiboot_info_t *mbi, int x, int y, int r, int g, int b);
void drawLine(multiboot_info_t *mbi, int x1, int y1, int x2, int y2, int r, int g, int b);
void changeBg(int r, int g, int b);
void changeTextColor(int r, int g, int b);

#endif