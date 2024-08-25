#include "disk.h"
#include "linux.h"
#include "types.h"

#ifndef FS_CONTROLLER_H
#define FS_CONTROLLER_H

typedef enum FS { FS_FATFS, FS_EXT2, FS_DEV, FS_SYS } FS;
typedef enum CONNECTOR {
  CONNECTOR_AHCI,
  CONNECTOR_DEV,
  CONNECTOR_SYS
} CONNECTOR;

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

typedef struct OpenFile OpenFile;

typedef int (*SpecialReadHandler)(OpenFile *fd, uint8_t *out, size_t limit);
typedef int (*SpecialWriteHandler)(OpenFile *fd, uint8_t *in, size_t limit);
typedef int (*SpecialIoctlHandler)(OpenFile *fd, uint64_t request, void *arg);
typedef int (*SpecialStatHandler)(OpenFile *fd, stat *stat);
typedef size_t (*SpecialMmapHandler)(size_t addr, size_t length, int prot,
                                     int flags, OpenFile *fd, size_t pgoffset);
typedef bool (*SpecialDuplicate)(OpenFile *original, OpenFile *orphan);
typedef int (*SpecialGetdents64)(OpenFile *fd, void *task,
                                 struct linux_dirent64 *dirp,
                                 unsigned int           count);
typedef bool (*SpecialOpen)(char *filename, OpenFile *fd);
typedef bool (*SpecialClose)(OpenFile *fd);

typedef struct VfsHandlers {
  SpecialReadHandler  read;
  SpecialWriteHandler write;
  SpecialIoctlHandler ioctl;
  SpecialStatHandler  stat;
  SpecialMmapHandler  mmap;
  SpecialGetdents64   getdents64;

  SpecialDuplicate duplicate;
  SpecialOpen      open;
  SpecialClose     close;
} VfsHandlers;

typedef struct MountPoint MountPoint;

typedef bool (*MntStat)(MountPoint *mnt, char *filename, struct stat *target);
typedef bool (*MntLstat)(MountPoint *mnt, char *filename, struct stat *target);
struct MountPoint {
  MountPoint *next;

  char *prefix;

  uint32_t  disk;
  uint8_t   partition; // mbr allows for 4 partitions / disk
  CONNECTOR connector;

  FS filesystem;

  VfsHandlers *handlers;
  MntStat      stat;
  MntLstat     lstat;

  mbr_partition mbr;
  void         *fsInfo;
};

struct OpenFile {
  OpenFile *next;

  int id;
  int flags;
  int mode;

  bool closeOnExec;

  char *dirname;

  size_t pointer;
  size_t tmp1;

  VfsHandlers *handlers;

  MountPoint *mountPoint;
  void       *dir;
  void       *fakefs;
};

MountPoint *firstMountPoint;

#define SEEK_SET 0  // start + offset
#define SEEK_CURR 1 // current + offset
#define SEEK_END 2  // end + offset

OpenFile *fsKernelOpen(char *filename, int flags, uint32_t mode);
bool      fsKernelClose(OpenFile *file);

int fsUserOpen(void *task, char *filename, int flags, int mode);
int fsUserClose(void *task, int fd);
int fsUserSeek(void *task, uint32_t fd, int offset, int whence);

OpenFile *fsUserGetNode(void *task, int fd);

OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original);
OpenFile *fsUserDuplicateNodeUnsafe(OpenFile *original);

uint32_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit);
uint32_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit);
bool     fsWriteSync(OpenFile *file);
void     fsReadFullFile(OpenFile *file, uint8_t *out);
int      fsReadlink(void *task, char *path, char *buf, int size);
uint32_t fsGetFilesize(OpenFile *file);

// vfs_sanitize.c
char *fsStripMountpoint(const char *filename, MountPoint *mnt);
char *fsSanitize(char *prefix, char *filename);

// vfs_stat.c
bool fsStat(OpenFile *fd, stat *target);
bool fsStatByFilename(void *task, char *filename, stat *target);
bool fsLstatByFilename(void *task, char *filename, stat *target);

// vfs_spec.c
bool   fsSpecificClose(OpenFile *file);
bool   fsSpecificOpen(char *filename, OpenFile *target);
int    fsSpecificRead(OpenFile *file, uint8_t *out, size_t limit);
int    fsSpecificWrite(OpenFile *file, uint8_t *in, size_t limit);
bool   fsSpecificWriteSync(OpenFile *file);
size_t fsSpecificGetFilesize(OpenFile *file);
bool   fsSpecificDuplicateNodeUnsafe(OpenFile *original, OpenFile *orphan);
int    fsSpecificSeek(OpenFile *file, int target, int offset, int whence);
int    fsSpecificIoctl(OpenFile *fd, uint64_t request, void *arg);
int    fsSpecificStat(OpenFile *fd, stat *target);
int    fsSpecificGetdents64(OpenFile *file, void *task,
                            struct linux_dirent64 *dirp, unsigned int count);

// vfs_mount.c
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition);
bool        fsUnmount(MountPoint *mnt);
MountPoint *fsDetermineMountPoint(char *filename);

#endif
