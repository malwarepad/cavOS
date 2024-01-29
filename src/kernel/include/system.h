#include "multiboot2.h"
#include "types.h"

#ifndef SYSTEM_H
#define SYSTEM_H

// Ports
uint8_t inportb(uint16_t _port);
void    outportb(uint16_t _port, uint8_t _data);

uint16_t inportw(uint16_t port);
void     outportw(unsigned short port, unsigned short data);

uint32_t inportl(uint16_t portid);
void     outportl(uint16_t portid, uint32_t value);

// Generic
void panic();

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
uint32_t framebuffer;
uint32_t framebuffer_end;
uint16_t framebufferWidth;
uint16_t framebufferHeight;
uint32_t framebufferPitch;

bool checkInterrupts();
void lockInterrupts();
void releaseInterrupts();

// Standard widespread functions
#define clearScreen drawClearScreen
void printfch(int character);

// Has root (system) drive been initialized?
bool systemDiskInit;

// Endianness
uint16_t switch_endian_16(uint16_t val);
uint32_t switch_endian_32(uint32_t val);

// From LD
uint32_t kernel_end;
uint32_t kernel_start;
uint32_t stack_bottom;

#endif
