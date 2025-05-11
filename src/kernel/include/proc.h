#include "fakefs.h"
#include "types.h"
#include "vfs.h"

#ifndef PROC_H
#define PROC_H

bool procMount(MountPoint *mount);

size_t procEachOpen(char *filename, int flags, int mode, OpenFile *fd,
                    char **symlinkResolve);
bool   procEachDuplicate(OpenFile *original, OpenFile *orphan);
bool   procEachClose(OpenFile *fd);

#endif
