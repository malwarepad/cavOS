#include "../../include/disk.h"

// Ata pio driver
// Copyright (C) 2022 Panagiotis

// void getDiskBytes(unsigned char *target, uint32_t LBA, uint8_t sector_count)
// { uint32_t *read = (uint32_t *)malloc((sector_count * SECTOR_SIZE) / 4);
// read_sectors_ATA_PIO(read, LBA, sector_count);

// uint16_t *target = (uint16_t *)target_address;

// int writingCurr = 0;
// for (int i = 0; i < ((sector_count * SECTOR_SIZE) / 4); i++) {
//   target[writingCurr] = read[i] & 0xFF;
//   target[writingCurr + 1] = (read[i] >> 8) & 0xFF;
//   target[writingCurr + 2] = (read[i] >> 16) & 0xFF;
//   target[writingCurr + 3] = (read[i] >> 24) & 0xFF;
//   writingCurr += 4;
// }

// read_sectors_ATA_PIO(target, LBA, sector_count);

// free(read);
// }

void putDiskBytes(const unsigned char *source, uint32_t LBA,
                  uint8_t sector_count) {
  uint32_t write_buffer[SECTOR_SIZE / 4]; // Create a buffer for 32-bit writes

  int writingCurr = 0;
  for (int i = 0; i < sector_count; i++) {
    for (int j = 0; j < SECTOR_SIZE / 4; j++) {
      // Combine 4 bytes into a 32-bit word
      write_buffer[j] = ((uint32_t)source[writingCurr]) |
                        (((uint32_t)source[writingCurr + 1]) << 8) |
                        (((uint32_t)source[writingCurr + 2]) << 16) |
                        (((uint32_t)source[writingCurr + 3]) << 24);
      writingCurr += 4;
    }
    // Write the 32-bit buffer to the disk
    write_sectors_ATA_PIO(LBA + i, 1, write_buffer);
  }
}
