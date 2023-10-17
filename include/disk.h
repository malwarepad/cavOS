#include "../include/ata.h"
#include "../src/boot/asm_ports/asm_ports.h"
#include "liballoc.h"
#include "types.h"
#include <stdint.h>

#define getDiskBytes read_sectors_ATA_PIO

#define SECTOR_SIZE 512 // we assume each sector is exactly 512 bytes long

#ifndef DISK_H
#define DISK_H

// void getDiskBytes(unsigned char *target, uint32_t LBA, uint8_t sector_count);
void putDiskBytes(const unsigned char *source, uint32_t LBA,
                  uint8_t sector_count);

#endif