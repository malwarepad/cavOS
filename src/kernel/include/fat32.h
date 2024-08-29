#include "types.h"
#include "vfs.h"

#ifndef FAT32_H
#define FAT32_H

// LFNs can have up to 255 characters. With each LFN having a hard maximum of 13
// characters, the maximum amount of LFNs per directory is ~19,61, say 20 for
// good measure.
#define LFN_MAX_CHARS_PER 13
#define LFN_MAX 20
#define LFN_MAX_TOTAL_CHARS 256
#define LFN_ORDER_FINAL 0x40

// File attributes
#define FAT_ATTRIB_READ_ONLY 0x01
#define FAT_ATTRIB_HIDDEN 0x02
#define FAT_ATTRIB_SYSTEM 0x04
#define FAT_ATTRIB_VOLUME_ID 0x08
#define FAT_ATTRIB_DIRECTORY 0x10
#define FAT_ATTRIB_ARCHIVE 0x20
#define FAT_ATTRIB_LFN                                                         \
  (FAT_ATTRIB_READ_ONLY | FAT_ATTRIB_HIDDEN | FAT_ATTRIB_SYSTEM |              \
   FAT_ATTRIB_VOLUME_ID)

#define FAT_PTR(a) ((FAT32 *)(a))
#define FAT_DIR_PTR(a) ((FAT32OpenFd *)(a))
#define FAT_COMB_HIGH_LOW(clusterhigh, clusterlow)                             \
  (((uint32_t)clusterhigh << 16) | clusterlow)

#define FAT_INODE_GEN(directory, index) ((directory) * 100 + (index))

typedef struct FAT32DirectoryEntry {
  char    name[8];
  char    ext[3];
  uint8_t attrib;
  uint8_t userattrib;

  char     undelete;
  uint16_t createtime;
  uint16_t createdate;
  uint16_t accessdate;
  uint16_t clusterhigh;

  uint16_t modifiedtime;
  uint16_t modifieddate;
  uint16_t clusterlow;
  uint32_t filesize;
} __attribute__((packed)) FAT32DirectoryEntry;

typedef struct FAT32ExtendedSector {
  uint32_t table_size_32;
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
} __attribute__((packed)) FAT32ExtendedSector;

typedef struct FAT32BootSector {
  uint8_t  bootjmp[3];
  uint8_t  oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t  sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t  table_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t  media_type;
  uint16_t table_size_16;
  uint16_t sectors_per_track;
  uint16_t head_side_count;
  uint32_t hidden_sector_count;
  uint32_t total_sectors_32;

  FAT32ExtendedSector extended_section;

} __attribute__((packed)) FAT32BootSector;

typedef struct FAT32LFN {
  uint8_t  order;
  uint8_t  first_five[10]; // first 5, 2-byte characters
  uint8_t  attributes;
  uint8_t  type; // indicates a sub-component of a long name (leave as 0)
  uint8_t  checksum;
  uint8_t  next_six[12]; // next 6, 2-byte characters
  uint16_t zero;         // must be zero - otherwise meaningless
  uint8_t  last_two[4];  // last 2, 2-byte characters
} __attribute__((packed)) FAT32LFN;
// fat->bootsec.table_count * fat->bootsec.extended_section.table_size_32

#define FAT32_CACHE_BAD 0xFFFFFFFF
#define FAT32_CACHE_MAX 32
typedef struct FAT32 {
  // various offsets
  size_t offsetBase;
  size_t offsetFats;
  size_t offsetClusters;

  // better "waste" some memory to be safe
  FAT32BootSector bootsec;

  // store old FATs for easier lookup
  uint8_t *cache[FAT32_CACHE_MAX];
  uint32_t cacheBase[FAT32_CACHE_MAX];
  int      cacheCurr;
} FAT32;

typedef struct FAT32OpenFd {
  uint32_t ptr;

  uint8_t  index; // x / 32
  uint32_t directoryStarting;
  uint32_t directoryCurr;

  FAT32DirectoryEntry dirEnt;
} FAT32OpenFd;

// fat32_controller.c
bool   fat32Mount(MountPoint *mount);
bool   fat32Open(char *filename, OpenFile *fd, char **symlinkResolve);
int    fat32Read(OpenFile *fd, uint8_t *buff, size_t limit);
size_t fat32Seek(OpenFile *fd, size_t target, long int offset, int whence);
size_t fat32GetFilesize(OpenFile *fd);
bool   fat32Close(OpenFile *fd);

// fat32_util.c
size_t        fat32ClusterToLBA(FAT32 *fat, uint32_t cluster);
int           fat32IsShortFilenamePossible(char *filename, size_t len);
void          fat32LFNmemcpy(uint8_t *lfnName, FAT32LFN *lfn, int index);
unsigned long fat32UnixTime(unsigned short fat_date, unsigned short fat_time);

// fat32_fat.c
uint32_t  fat32FATtraverse(FAT32 *fat, uint32_t offset);
uint32_t *fat32FATchain(FAT32 *fat, uint32_t offsetStart, uint32_t amount);

// fat32_traverse.c
typedef struct FAT32TraverseResult {
  uint32_t            directory;
  uint8_t             index; // x / 32
  FAT32DirectoryEntry dirEntry;
} FAT32TraverseResult;
FAT32TraverseResult fat32Traverse(FAT32 *fat, uint32_t initDirectory,
                                  char *search, size_t searchLength);
FAT32TraverseResult fat32TraversePath(FAT32 *fat, char *path,
                                      uint32_t directoryStarting);

// fat32_stat.c
bool fat32Stat(MountPoint *mnt, char *filename, struct stat *target,
               char **symlinkResolve);
int  fat32StatFd(OpenFile *fd, struct stat *target);

// fat32_dirs.c
int fat32Getdents64(OpenFile *file, struct linux_dirent64 *start,
                    unsigned int hardlimit);
int fat32SFNtoNormal(uint8_t *target, FAT32DirectoryEntry *dirent);

// finale
VfsHandlers fat32Handlers;

#endif
