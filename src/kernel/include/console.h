#include "types.h"

#ifndef CONSOLE_H
#define CONSOLE_H

uint32_t width;
uint32_t height;

extern int bg_color[];
extern int textcolor[];

#define TTY_CHARACTER_WIDTH 8
#define TTY_CHARACTER_HEIGHT 16

#define DEFAULT_FONT_PATH "/fonts/u_custom.psf"

void initiateConsole();
void drawCharacter(int charnum);

// Internal variables
void     changeBg(int r, int g, int b);
void     changeTextColor(int r, int g, int b);
uint32_t getConsoleX();
uint32_t getConsoleY();
void     setConsoleX(uint32_t x);
void     setConsoleY(uint32_t y);

// Control the console's pointer
void eraseBull();
void updateBull();

void clearScreen();

void printfch(char character);
void putchar_(char c);

#endif
