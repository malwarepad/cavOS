#ifndef DISK_H
#define DISK_H

#include "vga.h"
#include "util.h"
#include "multiboot.h"
#include "x86.h"

typedef struct {
    uint8 id;
    uint8 cylinders;
    uint8 sectors;
    uint8 heads;
} DISK;

uint8 DISK_Initialize(DISK* disk, uint8 driveNumber);
uint8 DISK_ReadSectors(DISK* disk, uint32 lba, uint8 sectors, void *dataOut);

void read_sectors_ATA_PIO(uint32 target_address, uint32 LBA, uint8 sector_count);
void write_sectors_ATA_PIO(uint32 LBA, uint8 sector_count, uint32* bytes);


#endif