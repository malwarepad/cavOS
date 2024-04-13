#include "types.h"

#ifndef SYSTEM_H
#define SYSTEM_H

void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

// Ports
uint8_t inportb(uint16_t _port);
void    outportb(uint16_t _port, uint8_t _data);

uint16_t inportw(uint16_t port);
void     outportw(unsigned short port, unsigned short data);

uint32_t inportl(uint16_t portid);
void     outportl(uint16_t portid, uint32_t value);

// Model Specific Registers (MSRs)
uint64_t rdmsr(uint32_t msrid);
uint64_t wrmsr(uint32_t msrid, uint64_t value);

// Streaming SIMD Extensions
void initiateSSE();

// Generic
void panic();

bool checkInterrupts();

// Has root (system) drive been initialized?
bool systemDiskInit;

// Executing syscall?
bool systemCallOnProgress;

// Endianness
uint16_t switch_endian_16(uint16_t val);
uint32_t switch_endian_32(uint32_t val);

// From LD
extern uint64_t kernel_start;
extern uint64_t kernel_end;
uint32_t        stack_bottom;

#endif
