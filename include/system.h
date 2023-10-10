#ifndef SYSTEM_H
#define SYSTEM_H
#include "multiboot2.h"
#include "types.h"

uint8 inportb(uint16 _port);
void  outportb(uint16 _port, uint8 _data);
void  sleep(uint8 times);

struct multiboot_tag *mbi;
unsigned long         mbi_addr;
unsigned              mbi_size;
unsigned int          mbi_memorySizeKb;
unsigned int          mbi_memorySize;

#define MAX_MEMORY_MAPS 100

multiboot_memory_map_t *memoryMap[MAX_MEMORY_MAPS];
int                     memoryMapCnt;

#endif
