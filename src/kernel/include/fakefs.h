#include "types.h"
#include "vfs.h"

#ifndef NULL_H
#define NULL_H

VfsHandlers handleNull;

typedef struct FakefsFile {
  struct FakefsFile *next;
  struct FakefsFile *inner;

  char *filename;
  int   filenameLength;

  uint16_t filetype;
  uint64_t inode;

  char *symlink;
  int   symlinkLength;

  int   size;
  void *extra;

  VfsHandlers *handlers;
} FakefsFile;

typedef struct Fakefs {
  FakefsFile *rootFile;
  uint64_t    lastInode;
} Fakefs;

typedef struct FakefsOverlay {
  Fakefs *fakefs;
} FakefsOverlay;

void        fakefsSetupRoot(FakefsFile **ptr);
FakefsFile *fakefsAddFile(Fakefs *fakefs, FakefsFile *under, char *filename,
                          char *symlink, uint16_t filetype,
                          VfsHandlers *handlers);
void        fakefsAttachFile(FakefsFile *file, void *ptr, int size);
bool        fakefsStat(MountPoint *mnt, char *filename, struct stat *target,
                       char **symlinkResolve);
bool        fakefsLstat(MountPoint *mnt, char *filename, struct stat *target,
                        char **symlinkResolve);
int         fakefsFstat(OpenFile *fd, stat *target);
int         fakefsSimpleRead(OpenFile *fd, uint8_t *out, size_t limit);

VfsHandlers fakefsHandlers;
VfsHandlers fakefsRootHandlers;
VfsHandlers fakefsSimpleReadHandlers;

#endif
