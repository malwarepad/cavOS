#include "../src/boot/asm_ports/asm_ports.h"
#include "disk.h"
#include "kb.h"
#include "liballoc.h"
#include "string.h"
#include "types.h"
#include "util.h"
#include <stdint.h>

#ifndef FAT32_H
#define FAT32_H

#define FAT_SIGNATURE1 0x28
#define FAT_SIGNATURE2 0x29

#define FAT_EOF 0x0ffffff8
#define FAT_DELETED 0xE5
#define PADDING_CHAR 0x20

typedef struct {
  uint8_t  bootjmp[3];
  uint8_t  oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t  sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t  number_of_fat;
  uint16_t root_entry_count;
  uint16_t sector_count_small;
  uint8_t  media_type;
  uint16_t sectors_per_fat_12_16;
  uint16_t sectors_per_track;
  uint16_t head_side_count;
  uint32_t hidden_sector_count;
  uint32_t sector_count;

  // extended fat32 stuff
  uint32_t sectors_per_fat;
  uint16_t extended_flags;
  uint16_t fat_version;
  uint32_t root_cluster;
  uint16_t fat_info;
  uint16_t backup_BS_sector;
  uint8_t  reserved_0[12];
  uint8_t  drive_number;
  uint8_t  reserved_1;
  uint8_t  boot_signature;
  uint32_t volume_id;
  uint8_t  volume_label[11];
  uint8_t  fat_type_label[8];

  // this will be cast to it's specific type once the driver actually knows what
  // type of FAT this is.
  // uint8_t extended_section[54];

  // extras
  uint32_t fat_begin_lba;
  uint32_t cluster_begin_lba;
  uint8_t  works;

} __attribute__((packed)) tmpFAT32, *ptmpFAT32;

#pragma pack(push, 1)
typedef struct FAT32_Directory {
  char     filename[11];
  uint8_t  attributes;
  uint8_t  ntReserved;
  uint8_t  creationTimeSeconds;
  uint16_t creationTime;
  uint16_t creationDate;
  uint16_t lastAccessDate;
  uint16_t firstClusterHigh;
  uint16_t lastModificationTime;
  uint16_t lastModificationDate;
  uint16_t firstClusterLow;
  uint32_t filesize;
} __attribute__((packed)) FAT32_Directory, *pFAT32_Directory;
#pragma pack(pop)

typedef struct {
  int          sector_count;
  unsigned int number_of_fat;
  unsigned int reserved_sectors;
  unsigned int sectors_per_track;
  unsigned int sectors_per_cluster;
  unsigned int sectors_per_fat;
  unsigned int volume_id;
  char         volume_label[12]; // 11 bytes, inc. 1 null terminator

  // helper
  int          works; // 1, 0
  unsigned int fat_begin_lba;
  unsigned int cluster_begin_lba;
} __attribute__((packed)) FAT32;

FAT32 fat;

int          initiateFat32();
int          showCluster(int clusterNum, int attrLimitation);
unsigned int getFatEntry(int cluster);
char        *formatToShort8_3Format(char *directory);
int          fileReaderTest();
int          openFile(pFAT32_Directory dir, char *filename);
char        *readFileContents(pFAT32_Directory dir);

#endif
