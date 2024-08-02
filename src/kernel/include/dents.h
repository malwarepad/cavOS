#include "linux.h"
#include "types.h"

#ifndef DENTS_H
#define DENTS_H

typedef enum {
  DENTS_NO_SPACE = 0,
  DENTS_SUCCESS = 1,
  DENTS_RETURN = 2,
} DENTS_RES;

DENTS_RES dentsAdd(void *buffStart, struct linux_dirent64 **dirp,
                   int *allocatedlimit, unsigned int hardlimit, char *filename,
                   size_t filenameLength, size_t inode, unsigned char type);

#endif