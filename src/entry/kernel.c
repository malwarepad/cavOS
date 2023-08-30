#include "../../include/kb.h"
#include "../../include/vga.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/multiboot.h"
#include "../../include/ata.h"
#include "../../include/disk.h"

#include <stdint.h>

// Kernel entry file
// Copyright (C) 2022 Panagiotis

int kmain(uint32 magic, multiboot_info_t *mbi)
{
    isr_install();
    clearScreen();
    printf("Welcome to cavOS! The OS that reminds you of how good computers \nwere back then.. Anyway, just execute any command you want\n'help' is your friend :)\n\nNote that this program comes with ABSOLUTELY NO WARRANTY.\n\n");

    // ! this is my testing land lol

    // printf("reading...\r\n");

    // uint32_t *target;

    // read_sectors_ATA_PIO(target, 0x0, 1); // LBA = Offset / Sector Size = 1048576 / 512 = 2048

    // unsigned char rawArr[512];
    // getDiskBytes(rawArr, 0, 1);

    // for (int i = 0; i < 512; i++)
    // {
    //     printf("%x ", rawArr[i]);
    // }

    // printf("\r\n");

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
