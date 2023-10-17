#include "../../include/ata.h"

// Ata pio driver
// Copyright (C) 2023 Panagiotis

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DRQ 0x08
#define STATUS_DF 0x20
#define STATUS_ERR 0x01

static void ATA_wait_BSY();
static void ATA_wait_DRQ();
void        read_sectors_ATA_PIO(uint8_t *target_address, uint32_t LBA,
                                 uint8_t sector_count) {

  ATA_wait_BSY();
  port_byte_out(0x1F6, 0xE0 | ((LBA >> 24) & 0xF));
  port_byte_out(0x1F2, sector_count);
  port_byte_out(0x1F3, (uint8_t)LBA);
  port_byte_out(0x1F4, (uint8_t)(LBA >> 8));
  port_byte_out(0x1F5, (uint8_t)(LBA >> 16));
  port_byte_out(0x1F7, 0x20); // Send the read command

  uint16_t *target = (uint16_t *)target_address;

  for (int j = 0; j < sector_count; j++) {
    ATA_wait_BSY();
    ATA_wait_DRQ();
    for (int i = 0; i < 256; i++)
      target[i] = port_word_in(0x1F0);
    target += 256;
  }
}

void write_sectors_ATA_PIO(uint32_t LBA, uint8_t sector_count,
                           uint32_t *bytes) {
  ATA_wait_BSY();
  port_byte_out(0x1F6, 0xE0 | ((LBA >> 24) & 0xF));
  port_byte_out(0x1F2, sector_count);
  port_byte_out(0x1F3, (uint8_t)LBA);
  port_byte_out(0x1F4, (uint8_t)(LBA >> 8));
  port_byte_out(0x1F5, (uint8_t)(LBA >> 16));
  port_byte_out(0x1F7, 0x30); // Send the write command

  for (int j = 0; j < sector_count; j++) {
    ATA_wait_BSY();
    ATA_wait_DRQ();
    for (int i = 0; i < 256; i++) {
      port_long_out(0x1F0, bytes[i]);
    }
  }
}

static void ATA_wait_BSY() // Wait for bsy to be 0
{
  while (port_byte_in(0x1F7) & STATUS_BSY)
    ;
}
static void ATA_wait_DRQ() // Wait fot drq to be 1
{
  while (!(port_byte_in(0x1F7) & STATUS_RDY))
    ;
}