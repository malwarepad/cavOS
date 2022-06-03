#ifndef BARE_H
#define BARE_H

#include "vga.h"
#include "util.h"
#include "multiboot.h"

#define MAX_WND 5

struct window
{
    int id;
    string title;
    int w;
    int h;
    int x;
    int y;

    int text_w;
    int text_h;
};

extern struct window windows[MAX_WND];

void startCWM(multiboot_info_t *mbi);
int setBackground(multiboot_info_t *mbi, int color[]);
int initializeWnd(multiboot_info_t *mbi, string title, int id, int x, int y, int w, int h);
void DesignTool_Text(multiboot_info_t *mbi, int id, const char *format, ...);
string DesignTool_KbInput(multiboot_info_t *mbi, int id);
void closeCWM(multiboot_info_t *mbi);

#endif // BARE_H
