#include "../src/boot/asm_ports/asm_ports.h"
#include "allocation.h"
#include "disk.h"
#include "string.h"
#include "types.h"
#include "util.h"
#include <stdint.h>

#ifndef FAT32_H
#define FAT32_H

#define FAT_EOF 0x0ffffff8

enum {
  // DIRENT_FILE,
  // DIRENT_EXTFILENAME,
  // DIRENT_VOLLABEL,
  // DIRENT_BLANK,
  DIRENT_END
};

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

// typedef struct
// {
// 	unsigned int sector_number;
// 	unsigned int entry_index;
// 	unsigned int raw_28_bits;
// 	bool is_last;
// } FAT32_FAT;

typedef struct {
  char *filename; // 11 bytes, inc. 1 null terminator
  uint8 attr;     // todo
  uint8 size;
  //   unsigned int createdDate;
  //   int    NTreserved; // nah but why NT-only?
  uint8  createdDay;
  uint8  createdMonth;
  uint16 createdYear;

  // unsigned int rawBits[32];
} __attribute__((packed)) FAT32_FILE;

int          initiateFat32();
int          showCluster(int clusterNum, int attrLimitation);
unsigned int getFatEntry(int cluster);
int          highLowCombiner(uint16_t highBits[2], uint16_t lowBits[2]);
int          showFileByCluster(int clusterNum, int size);

#endif
