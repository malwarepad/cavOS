#include "disk.h"
#include "types.h"

#ifndef FS_CONTROLLER_H
#define FS_CONTROLLER_H

typedef enum FS { FS_FAT32 } FS;
typedef enum CONNECTOR { CONNECTOR_ATAPIO } CONNECTOR;

typedef struct MountPoint MountPoint;
struct MountPoint {
  char *prefix;

  uint32_t  disk;
  uint8_t   partition; // mbr allows for 4 partitions / disk
  CONNECTOR connector;

  FS filesystem;

  mbr_partition mbr;
  void         *fsInfo;

  MountPoint *next;
};

typedef struct OpenFile OpenFile;
struct OpenFile {
  int id;

  uint32_t pointer;
  uint32_t tmp1;

  MountPoint *mountPoint;
  void       *dir;

  OpenFile *next;
};

OpenFile *firstKernelFile;

MountPoint *firstMountPoint;

MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition);
bool        fsUnmount(MountPoint *mnt);
MountPoint *fsDetermineMountPoint(char *filename);

OpenFile *fsKernelOpen(char *filename);
bool      fsKernelClose(OpenFile *file);

int fsUserOpen(char *filename, int flags, uint16_t mode);
int fsUserClose(int fd);
int fsUserSeek(uint32_t fd, int offset, int whence);

uint32_t fsRead(OpenFile *file, char *out, uint32_t limit);
void     fsReadFullFile(OpenFile *file, char *out);
uint32_t fsGetFilesize(OpenFile *file);

#endif
