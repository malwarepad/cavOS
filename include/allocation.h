#include "multiboot.h"
#include "types.h"

#ifndef ALLOCATION_H
#define ALLOCATION_H

void  initialiseMemory(uint32 magic, multiboot_info_t *mbi);
void *malloc(size_t size);
void  free(void *ptr);

#endif