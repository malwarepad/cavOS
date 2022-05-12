#include "../../../include/vga.h"
#include "../../../include/multiboot.h"
#include "../../../include/kb.h"
#include "../../../include/wm.h"
#include "../../../include/sysinfo.h"

// Terminal application for CWM (Cave-Like Window Manager)
// Copyright (C) 2022 Panagiotis

void StartTerminal(multiboot_info_t *mbi, int id) {
    string ch = (string) malloc(200);
    string data[64];
    string prompt = "$ ";
    int reading = 1;
    DesignTool_Text(mbi, id, "This app and the Cave-Like Window Manager are still a work in progress! I'll make them better in newer releases.\n\n");
    do
    {
        DesignTool_Text(mbi, id, prompt);
        ch = DesignTool_KbInput(mbi, id); //memory_copy(readStr(mbi), ch,100);
        if(cmdEql(ch,"clear"))
        {
            term_clear(mbi, id);
        }
        else if(cmdEql(ch,"echo"))
        {
            term_echo(mbi, id, ch);
        }
        else if(cmdEql(ch,"help"))
        {
            term_help(mbi, id);
        }
        else if(cmdEql(ch,"fetch"))
        {
            term_fetch(mbi, id);
        }
        else if(cmdEql(ch,"credits") || cmdEql(ch,"cxt"))
        {
            term_credits(mbi, id);
        }
        else
        {
            DesignTool_Text(mbi, id, "\n%s isn't a valid command\n", ch);
            if(check_string(ch) && !cmdEql(ch,"exit")) {
            } else {
                DesignTool_Text(mbi, id, "\n");
                closeCWM(mbi);
                reading = 0;
            }
        }
    } while (reading == 1);
}

void term_credits(multiboot_info_t *mbi, int id) {
    initializeWnd(mbi, "Credits", 1, 850, 20, 400, 500);
    DesignTool_Text(mbi, 1, "Special thanks to:\n- Panagiotis (Maintainer, Head Developer)\n- Jonas (Developer)");
    DesignTool_Text(mbi, id, "\n");
}

void term_clear(multiboot_info_t *mbi, int id) {
    drawRect(mbi, windows[id].x, windows[id].y+25, windows[id].w, windows[id].h-25, 255, 255, 255);
    width = windows[id].x;
    height = windows[id].y+25;
    windows[id].text_w = windows[id].x;
    windows[id].text_h = windows[id].y+25;
}

void term_echo(multiboot_info_t *mbi, int id, string ch)
{
    if (argSrch(ch, 1, 1) != 0) {
        DesignTool_Text(mbi, id, "\n%s\n", argSrch(ch, 1, 1));
    }
    else {
        DesignTool_Text(mbi, id, "\nEcho requires an argument!\n");
    }
}

void term_fetch(multiboot_info_t *mbi, int id) {
    DesignTool_Text(mbi, id, "\nos: cavOS %s", cavos_version);
    DesignTool_Text(mbi, id, "\nshell: csh %s", cavos_shell);
    DesignTool_Text(mbi, id, "\nresolution: %dx%d", mbi->framebuffer_width, mbi->framebuffer_height);
    DesignTool_Text(mbi, id, "\nmemory: %dMB", ((mbi->mem_upper) / 1024));
    DesignTool_Text(mbi, id, "\n");
}

void term_help(multiboot_info_t *mbi, int id)
{
    DesignTool_Text(mbi, id, "\ncmd       : Launch a new recursive Shell");
    DesignTool_Text(mbi, id, "\nclear     : Clears the screen");
    DesignTool_Text(mbi, id, "\necho      : Reprintf a given text");
    DesignTool_Text(mbi, id, "\nexit      : Quits the current shell");
    DesignTool_Text(mbi, id, "\nfetch     : Brings you some system information");

    DesignTool_Text(mbi, id, "\n");
}
