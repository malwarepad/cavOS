#include "../../include/kb.h"
#include "../../include/vga.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/multiboot.h"
#include "../../include/ata.h"

#include <stdint.h>

// Kernel entry file
// Copyright (C) 2022 Panagiotis

int kmain(uint32 magic, multiboot_info_t *mbi)
{
    (void)magic;
    isr_install();
    clearScreen();
    printf("Welcome to cavOS! The OS that reminds you of how good computers \nwere back then.. Anyway, just execute any command you want\n'help' is your friend :)\n\nNote that this program comes with ABSOLUTELY NO WARRANTY.\n\n");

    /*printf("reading...\r\n");

    uint32_t *target;

    read_sectors_ATA_PIO(target, 0x0, 1);

    int i;
    i = 0;
    while (i < 128)
    {
        printf("%x ", target[i] & 0xFF);
        printf("%x ", (target[i] >> 8) & 0xFF);
        i++;
    }

    printf("\r\n");*/

    /*printf("writing 0...\r\n");
    char bwrite[512];
    for(i = 0; i < 512; i++)
    {
        bwrite[i] = 0x0;
    }
    write_sectors_ATA_PIO(0x0, 2, bwrite);


    printf("reading...\r\n");
    read_sectors_ATA_PIO(target, 0x0, 1);

    i = 0;
    while(i < 128)
    {
        printf("%x ", target[i] & 0xFF);
        printf("%x ", (target[i] >> 8) & 0xFF);
        i++;
    }
    printf("\n");*/
    launch_shell(0, mbi);
}
