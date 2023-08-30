#include "types.h"
#include <stdint.h>
#include "../src/boot/asm_ports/asm_ports.h"
#include "../include/ata.h"

#define SECTOR_SIZE 512 // we assume each sector is exactly 512 bytes long

void getDiskBytes(unsigned char *target, uint32_t LBA, uint8_t sector_count);