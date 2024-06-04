#include "disk.h"
#include "types.h"

#ifndef FS_CONTROLLER_H
#define FS_CONTROLLER_H

typedef enum FS { FS_FATFS } FS;
typedef enum CONNECTOR { CONNECTOR_AHCI } CONNECTOR;

// Accordingly to fatfs
#define FS_MODE_READ 0x01
#define FS_MODE_WRITE 0x02
#define FS_MODE_OPEN_EXISTING 0x00
#define FS_MODE_CREATE_NEW 0x04
#define FS_MODE_CREATE_ALWAYS 0x08
#define FS_MODE_OPEN_ALWAYS 0x10
#define FS_MODE_OPEN_APPEND 0x30

// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h
#define O_ACCMODE 00000003
#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#define O_CREAT 00000100  /* not fcntl */
#define O_EXCL 00000200   /* not fcntl */
#define O_NOCTTY 00000400 /* not fcntl */
#define O_TRUNC 00001000  /* not fcntl */
#define O_APPEND 00002000
#define O_NONBLOCK 00004000
#define O_DSYNC 00010000  /* used to be O_SYNC, see below */
#define FASYNC 00020000   /* fcntl, for BSD compatibility */
#define O_DIRECT 00040000 /* direct disk access hint */
#define O_LARGEFILE 00100000
#define O_DIRECTORY 00200000 /* must be a directory */
#define O_NOFOLLOW 00400000  /* don't follow links */
#define O_NOATIME 01000000
#define O_CLOEXEC 02000000 /* set close_on_exec */
#define __O_SYNC 04000000
#define O_SYNC (__O_SYNC | O_DSYNC)
#define O_PATH 010000000
#define __O_TMPFILE 020000000
#define O_TMPFILE (__O_TMPFILE | O_DIRECTORY)
#define O_NDELAY O_NONBLOCK

typedef struct stat {
  uint32_t st_dev;     // Device ID
  uint32_t st_ino;     // inode number
  uint32_t st_mode;    // File mode
  uint32_t st_nlink;   // Number of hard links
  uint32_t st_uid;     // User ID of owner
  uint32_t st_gid;     // Group ID of owner
  uint32_t st_rdev;    // Device ID (if special file)
  uint32_t st_size;    // Total size, in bytes
  uint32_t st_blksize; // Optimal block size for I/O
  uint32_t st_blocks;  // Number of 512B blocks allocated
  uint64_t st_atime;   // Time of last access
  uint64_t st_mtime;   // Time of last modification
  uint64_t st_ctime;   // Time of last status change
} stat;

typedef struct stat_extra {
  bool file;
} stat_extra;

typedef struct MountPoint MountPoint;
struct MountPoint {
  MountPoint *next;

  char *prefix;

  uint32_t  disk;
  uint8_t   partition; // mbr allows for 4 partitions / disk
  CONNECTOR connector;

  FS filesystem;

  mbr_partition mbr;
  void         *fsInfo;
};

// mount point for "special" files ;)
#define MOUNT_POINT_SPECIAL ((MountPoint *)0x69)
typedef struct OpenFile OpenFile;
struct OpenFile {
  OpenFile *next;

  int      id;
  int      flags;
  uint32_t mode;

  char *safeFilename;

  size_t pointer;
  size_t tmp1;

  MountPoint *mountPoint;
  void       *dir;
};

typedef struct SpecialFile SpecialFile;
typedef int (*SpecialReadHandler)(OpenFile *fd, uint8_t *out, size_t limit);
typedef int (*SpecialWriteHandler)(OpenFile *fd, uint8_t *in, size_t limit);
typedef int (*SpecialIoctlHandler)(OpenFile *fd, uint64_t request, void *arg);
typedef size_t (*SpecialMmapHandler)(size_t addr, size_t length, int prot,
                                     int flags, int fd, size_t pgoffset);

typedef struct SpecialHandlers {
  SpecialReadHandler  read;
  SpecialWriteHandler write;
  SpecialIoctlHandler ioctl;
  SpecialMmapHandler  mmap;
} SpecialHandlers;

struct SpecialFile {
  SpecialFile *next;

  int   id;
  char *filename;

  SpecialHandlers *handlers;
};

OpenFile *firstKernelFile;

MountPoint *firstMountPoint;

MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition);
bool        fsUnmount(MountPoint *mnt);
MountPoint *fsDetermineMountPoint(char *filename);

OpenFile *fsKernelOpen(char *filename, int flags, uint32_t mode);
bool      fsKernelClose(OpenFile *file);

int fsUserOpen(char *filename, int flags, uint32_t mode);
int fsUserClose(int fd);
int fsUserSeek(uint32_t fd, int offset, int whence);

OpenFile *fsUserGetNode(int fd);

bool         fsUserOpenSpecial(char *filename, void *taskPtr, int fd,
                               SpecialHandlers *specialHandlers);
bool         fsUserCloseSpecial(SpecialFile *special);
SpecialFile *fsUserGetSpecialByFilename(char *filename);
SpecialFile *fsUserGetSpecialById(void *taskPtr, int fd);

OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original);
OpenFile *fsUserDuplicateNodeUnsafe(OpenFile *original, SpecialFile *special);
SpecialFile *fsUserDuplicateSpecialNodeUnsafe(SpecialFile *original);

uint32_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit);
uint32_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit);
bool     fsWriteChar(OpenFile *file, char in);
bool     fsWriteSync(OpenFile *file);
void     fsReadFullFile(OpenFile *file, uint8_t *out);
uint32_t fsGetFilesize(OpenFile *file);
bool     fsStat(char *filename, stat *target, stat_extra *extra);

#endif
