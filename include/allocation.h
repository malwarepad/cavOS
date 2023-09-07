#include "multiboot.h"
#include "types.h"

#ifndef ALLOCATION_H
#define ALLOCATION_H

void *malloc(size_t size);
void  free(void *ptr);

#endif