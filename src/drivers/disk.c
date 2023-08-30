#include "../../include/disk.h"

// Ata pio driver
// Copyright (C) 2022 Panagiotis

void getDiskBytes(unsigned char *target, uint32_t LBA, uint8_t sector_count)
{
	uint32_t *read;
	read_sectors_ATA_PIO(read, LBA, sector_count);

	// uint16_t *target = (uint16_t *)target_address;

	int writingCurr = 0;
	for (int i = 0; i < ((sector_count * SECTOR_SIZE) / 4); i++)
	{
		target[writingCurr] = read[i] & 0xFF;
		target[writingCurr + 1] = (read[i] >> 8) & 0xFF;
		target[writingCurr + 2] = (read[i] >> 16) & 0xFF;
		target[writingCurr + 3] = (read[i] >> 24) & 0xFF;
		writingCurr += 4;
	}
}