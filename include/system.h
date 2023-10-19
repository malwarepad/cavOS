#include "multiboot2.h"
#include "types.h"

#ifndef SYSTEM_H
#define SYSTEM_H

// Ports
uint8 inportb(uint16 _port);
void  outportb(uint16 _port, uint8 _data);

uint32_t inportl(uint16_t portid);
void     outportl(uint16_t portid, uint32_t value);

// GRUB-passed memory info
struct multiboot_tag *mbi;
unsigned long         mbi_addr;
unsigned              mbi_size;
unsigned int          mbi_memorySizeKb;
unsigned int          mbi_memorySize;

#define MAX_MEMORY_MAPS 100

multiboot_memory_map_t *memoryMap[MAX_MEMORY_MAPS];
int                     memoryMapCnt;

// Graphical framebuffer
uint32_t *framebuffer;
uint16_t  framebufferWidth;
uint16_t  framebufferHeight;
uint32_t  framebufferPitch;

// Standard widespread functions
#define clearScreen drawClearScreen
void printfch(int character);

#endif
