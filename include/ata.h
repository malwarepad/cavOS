#include "../src/boot/asm_ports/asm_ports.h"
#include "types.h"
#include <stdint.h>

#ifndef ATA_H
#define ATA_H

void read_sectors_ATA_PIO(uint32_t target_address, uint32_t LBA,
                          uint8_t sector_count);
void write_sectors_ATA_PIO(uint32_t LBA, uint8_t sector_count, uint32_t *bytes);

#endif
