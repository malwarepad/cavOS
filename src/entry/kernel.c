#include "../../include/kb.h"
#include "../../include/vga.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/multiboot.h"
#include "../../include/wm.h"

#include "../wm/software/terminal.h"

// Kernel entry file
// Copyright (C) 2022 Panagiotis

int kmain(uint32 magic, multiboot_info_t *mbi)
{
	(void)magic;
	isr_install(mbi);
	clearScreen(mbi);
    changeTextColor(255, 255, 255);
    changeBg(0, 0, 0);
	printf(mbi, "Welcome to cavOS! The OS that reminds you of how good computers \nwere back then.. Anyway, just execute any command you want\n'help' is your friend :)\n\nNote that this program comes with ABSOLUTELY NO WARRANTY.\n\n");
	launch_shell(0, mbi);
}
