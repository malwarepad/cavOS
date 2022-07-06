#include "../../include/kb.h"
#include "../../include/vga.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/multiboot.h"
#include "../../include/wm.h"

#include "../wm/software/terminal.h"

#include "../../include/disk.h"
#include "../../include/fat.h"
#include "../../include/x86.h"

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

    /*uint32* target;
    read_sectors_ATA_PIO(target, 0x0, 2);

    for (int i = 0; i < 128; i++) {
        printf(mbi, "%d", target[i] & 0xFF);
        printf(mbi, "%d", (target[i] >> 8) & 0xFF);
        printf(mbi, " ");
    }*/


    /*if (!FAT_Initialize(&disk)) {
        printf(mbi, "error \n");
        while (1==1) {}
    }*/

    /*FAT_File* fd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    while (FAT_ReadEntry(&disk, fd, &entry)) {
        printf(mbi, "");
        for (int i = 0; i < 11; i++) {
            printf(mbi, "%c", entry.Name[i]);
        }
        printf(mbi, "\n");
    }
    FAT_Close(fd);*/

    while (1==1) {}
}
