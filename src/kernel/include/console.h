#include "types.h"

#ifndef CONSOLE_H
#define CONSOLE_H

#define TTY_CHARACTER_WIDTH 8
#define TTY_CHARACTER_HEIGHT 16

#define DEFAULT_FONT_PATH "/fonts/u_vga16.sfn"

void initiateConsole();
void drawCharacter(int charnum);
void changeBg(int r, int g, int b);
void changeTextColor(int r, int g, int b);
void drawClearScreen();

#endif
