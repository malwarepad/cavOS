#include "multiboot2.h"
#include "types.h"

#ifndef ALLOCATION_H
#define ALLOCATION_H

void  initialiseMemory(uint32 magic);
void *malloc(size_t size);
void  free(void *ptr);

#endif