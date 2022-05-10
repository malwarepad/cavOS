#include "../../include/vga.h"
#include "../../include/multiboot.h"
#include "../../include/kb.h"
#include "software/terminal.h"
#include <stdarg.h>

// Bare-bones GUI library (base for more complex infostructure)
// Files inside the src/wm directory are meant to be
// used for CWM, the Cave-Like Window Manager.
// Copyright (C) 2022 Panagiotis

/* Persistent memory variables */

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

struct window windows[MAX_WND];

// int windowText(multiboot_info_t *mbi, int id, int color[], string str1)

string DesignTool_KbInput(multiboot_info_t *mbi, int id) {
    if (windows[id].text_h == 0)
    {
        windows[id].text_h = windows[id].y + 25;
    }

    if (windows[id].text_w == 0)
    {
        windows[id].text_w = windows[id].x;
    }

    width = windows[id].text_w;
    height = windows[id].text_h;

    string str1 = readStr(mbi);

    windows[id].text_h = height;
    windows[id].text_w = width;

    return str1;
}

void DesignTool_Text(multiboot_info_t *mbi, int id, const char *format, ...)
{
    if (windows[id].text_h == 0)
    {
        windows[id].text_h = windows[id].y + 25;
    }

    if (windows[id].text_w == 0)
    {
        windows[id].text_w = windows[id].x;
    }

    width = windows[id].text_w;
    height = windows[id].text_h;
    va_list ap;
    va_start(ap, format);

    uint8 *ptr;

    for (ptr = format; *ptr != '\0'; ptr++)
    {
        if (*ptr == '\n')
        {
            height += 16;
            width = windows[id].x;
        }
        else
        {
            if (*ptr == '%')
            {
                ptr++;
                switch (*ptr)
                {
                case 's':
                    printf(mbi, va_arg(ap, uint8 *));
                    break;
                    // case 'c':
                    // printf(char_to_string(va_arg(ap, uint8 *)));
                case 'd':
                    printf(mbi, int_to_string(va_arg(ap, uint8 *)));
                    break;
                case '%':
                    drawText(mbi, '%');
                    break;
                }
            }
            else
            {
                if ((height - windows[id].y) > (windows[id].h - 25)) {
                    term_clear(mbi, id); /* temporary solution as the only app is a terminal one */
                    /* ! If you are a dev, wait for the next release to create GUI software for */
                    /* cavOS */
                }
                if ((width - windows[id].x) > (windows[id].w - 25)) {
                    height += 16;
                    width = windows[id].x;
                }
                drawText(mbi, *ptr);
            }
        }
    }

    va_end(ap);

    windows[id].text_h = height;
    windows[id].text_w = width;
}

int setBackground(multiboot_info_t *mbi, int color[])
{
    drawRect(mbi, 0, 0, mbi->framebuffer_width, mbi->framebuffer_height, color[0], color[1], color[2]);

    return 0;
}

void startCWM(multiboot_info_t *mbi) {
    int blue[] = {0, 0, 255};
    changeTextColor(0, 0, 0);
    changeBg(255, 255, 255);
    setBackground(mbi, blue);
    initializeWnd(mbi, "Terminal", 0, 20, 20, 800, 650);
    StartTerminal(mbi, 0);
}

void closeCWM(multiboot_info_t *mbi) {
    for (int i = 0; i < MAX_WND; i++) {
        windows[i].id = 0;
        windows[i].title = "";
        windows[i].w = 0;
        windows[i].h = 0;
        windows[i].x = 0;
        windows[i].y = 0;

        windows[i].text_w = 0;
        windows[i].text_h = 0;
    }
    width = 0;
    height = 0;
    int bg[] = {0, 0, 0};
    changeBg(0, 0, 0);
    changeTextColor(255, 255, 255);
    setBackground(mbi, bg);
}

int drawWnd(multiboot_info_t *mbi, int id)
{
    int w = windows[id].w;
    int h = windows[id].h;
    int x = windows[id].x;
    int y = windows[id].y;
    string title = windows[id].title;
    drawRect(mbi, x, y, w, h, 255, 255, 255);
    drawRect(mbi, x, y, w, 25, 189, 189, 189);
    width = x;
    height = y;
    printf(mbi, title);

    /* Close button */
    drawLine(mbi, x + w - 25, y, x + w - 25, y + 25, 0, 0, 0);
    drawLine(mbi, x + w - 25, y, x + w, y, 0, 0, 0);
    drawLine(mbi, x + w - 25, y + 25, x + w, y + 25, 0, 0, 0);
    drawLine(mbi, x + w, y, x + w, y + 25, 0, 0, 0);

    /* Close button: cross */
    drawLine(mbi, x + w - 20, y + 5, x + w - 5, y + 20, 0, 0, 0);
    drawLine(mbi, x + w - 20, y + 20, x + w - 5, y + 5, 0, 0, 0);

    return 0;
}

int closeWnd(multiboot_info_t *mbi, int id)
{
    drawRect(mbi, windows[id].x, windows[id].y, windows[id].w + 1, windows[id].h, 0, 0, 255);
}

int initializeWnd(multiboot_info_t *mbi, string title, int id, int x, int y, int w, int h)
{
    windows[id].title = title;
    windows[id].h = h;
    windows[id].w = w;
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;

    int arr[] = {0, 0, 255};
    // setBackground(mbi, arr);
    drawWnd(mbi, id);
    width = windows[id].x;
    height = windows[id].y + 25;
    // printf(mbi, "%d %d", windows[id].x, windows[id].y);
    //DesignTool_Text(mbi, id, "%d %d \n", windows[id].x, windows[id].y);
    //DesignTool_Text(mbi, id, "%d fsdf %d", windows[id].x, windows[id].y);
    return 0;
}
