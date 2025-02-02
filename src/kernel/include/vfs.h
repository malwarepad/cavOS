#include "disk.h"
#include "linux.h"
#include "types.h"

#ifndef FS_CONTROLLER_H
#define FS_CONTROLLER_H

typedef enum FS { FS_FATFS, FS_EXT2, FS_DEV, FS_SYS, FS_PROC } FS;
typedef enum CONNECTOR {
  CONNECTOR_AHCI,
  CONNECTOR_DEV,
  CONNECTOR_SYS,
  CONNECTOR_PROC
} CONNECTOR;

// Accordingly to fatfs
// #define FS_MODE_READ 0x01
// #define FS_MODE_WRITE 0x02
// #define FS_MODE_OPEN_EXISTING 0x00
// #define FS_MODE_CREATE_NEW 0x04
// #define FS_MODE_CREATE_ALWAYS 0x08
// #define FS_MODE_OPEN_ALWAYS 0x10
// #define FS_MODE_OPEN_APPEND 0x30

// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h
// #define O_ACCMODE 00000003
// #define O_RDONLY 00000000
// #define O_WRONLY 00000001
// #define O_RDWR 00000002
// #define O_CREAT 00000100  /* not fcntl */
// #define O_EXCL 00000200   /* not fcntl */
// #define O_NOCTTY 00000400 /* not fcntl */
// #define O_TRUNC 00001000  /* not fcntl */
// #define O_APPEND 00002000
// #define O_NONBLOCK 00004000
// #define O_DSYNC 00010000  /* used to be O_SYNC, see below */
// #define FASYNC 00020000   /* fcntl, for BSD compatibility */
// #define O_DIRECT 00040000 /* direct disk access hint */
// #define O_LARGEFILE 00100000
// #define O_DIRECTORY 00200000 /* must be a directory */
// #define O_NOFOLLOW 00400000  /* don't follow links */
// #define O_NOATIME 01000000
// #define O_CLOEXEC 02000000 /* set close_on_exec */
// #define __O_SYNC 04000000
// #define O_SYNC (__O_SYNC | O_DSYNC)
// #define O_PATH 010000000
// #define __O_TMPFILE 020000000
// #define O_TMPFILE (__O_TMPFILE | O_DIRECTORY)
// #define O_NDELAY O_NONBLOCK

typedef struct OpenFile OpenFile;

typedef size_t (*SpecialReadHandler)(OpenFile *fd, uint8_t *out, size_t limit);
typedef size_t (*SpecialWriteHandler)(OpenFile *fd, uint8_t *in, size_t limit);
typedef size_t (*SpecialSeekHandler)(OpenFile *file, size_t target,
                                     long int offset, int whence);
typedef size_t (*SpecialIoctlHandler)(OpenFile *fd, uint64_t request,
                                      void *arg);
typedef size_t (*SpecialStatHandler)(OpenFile *fd, stat *stat);
typedef size_t (*SpecialMmapHandler)(size_t addr, size_t length, int prot,
                                     int flags, OpenFile *fd, size_t pgoffset);
typedef bool (*SpecialDuplicate)(OpenFile *original, OpenFile *orphan);
typedef size_t (*SpecialGetdents64)(OpenFile *fd, struct linux_dirent64 *dirp,
                                    unsigned int count);
typedef size_t (*SpecialOpen)(char *filename, int flags, int mode, OpenFile *fd,
                              char **symlinkResolve);
typedef bool (*SpecialClose)(OpenFile *fd);
typedef size_t (*SpecialGetFilesize)(OpenFile *fd);
typedef void (*SpecialFcntl)(OpenFile *fd, int cmd, uint64_t arg);
typedef bool (*SpecialPoll)(OpenFile *fd, struct pollfd *pollFd, int timeout);

typedef struct VfsHandlers {
  SpecialReadHandler  read;
  SpecialWriteHandler write;
  SpecialSeekHandler  seek;
  SpecialIoctlHandler ioctl;
  SpecialStatHandler  stat;
  SpecialMmapHandler  mmap;
  SpecialGetdents64   getdents64;
  SpecialGetFilesize  getFilesize;
  SpecialPoll         poll;
  SpecialFcntl        fcntl; // it's extra

  SpecialDuplicate duplicate;
  SpecialOpen      open;
  SpecialClose     close;
} VfsHandlers;

typedef struct MountPoint MountPoint;

typedef bool (*MntStat)(MountPoint *mnt, char *filename, struct stat *target,
                        char **symlinkResolve);
typedef bool (*MntLstat)(MountPoint *mnt, char *filename, struct stat *target,
                         char **symlinkResolve);

typedef size_t (*MntMkdir)(MountPoint *mnt, char *path, uint32_t mode,
                           char **symlinkResolve);
typedef size_t (*MntDelete)(MountPoint *mnt, char *path, bool directory,
                            char **symlinkResolve);

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
  MntMkdir     mkdir;
  MntDelete delete;

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

size_t fsUserOpen(void *task, char *filename, int flags, int mode);
size_t fsUserClose(void *task, int fd);
size_t fsUserSeek(void *task, uint32_t fd, int offset, int whence);

OpenFile *fsUserGetNode(void *task, int fd);

OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original);
OpenFile *fsUserDuplicateNodeUnsafe(OpenFile *original);

size_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit);
size_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit);
void   fsReadFullFile(OpenFile *file, uint8_t *out);
size_t fsReadlink(void *task, char *path, char *buf, int size);
size_t fsMkdir(void *task, char *path, uint32_t mode);
size_t fsUnlink(void *task, char *path, bool directory);
size_t fsGetFilesize(OpenFile *file);

// vfs_sanitize.c
char *fsStripMountpoint(const char *filename, MountPoint *mnt);
char *fsSanitize(char *prefix, char *filename);

// vfs_stat.c
bool fsStat(OpenFile *fd, stat *target);
bool fsStatByFilename(void *task, char *filename, stat *target);
bool fsLstatByFilename(void *task, char *filename, stat *target);

// vfs_mount.c
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition);
bool        fsUnmount(MountPoint *mnt);
MountPoint *fsDetermineMountPoint(char *filename);
char       *fsResolveSymlink(MountPoint *mnt, char *symlink);

#endif
