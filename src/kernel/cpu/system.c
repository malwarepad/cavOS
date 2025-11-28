#include <bootloader.h>
#include <console.h>
#include <system.h>

// Source code for handling ports via assembly references
// Copyright (C) 2024 Panagiotis

void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
  asm volatile("cpuid \n"
               : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
               : "a"(*eax), "c"(*ecx)
               : "memory");
}

uint8_t inportb(uint16_t _port) {
  uint8_t rv;
  __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "dN"(_port));
  return rv;
}

void outportb(uint16_t _port, uint8_t _data) {
  __asm__ __volatile__("outb %1, %0" : : "dN"(_port), "a"(_data));
}

uint16_t inportw(uint16_t port) {
  uint16_t result;
  __asm__("in %%dx, %%ax" : "=a"(result) : "d"(port));
  return result;
}

void outportw(unsigned short port, unsigned short data) {
  __asm__("out %%ax, %%dx" : : "a"(data), "d"(port));
}

uint32_t inportl(uint16_t portid) {
  uint32_t ret;
  __asm__ __volatile__("inl %%dx, %%eax" : "=a"(ret) : "d"(portid));
  return ret;
}

void outportl(uint16_t portid, uint32_t value) {
  __asm__ __volatile__("outl %%eax, %%dx" : : "d"(portid), "a"(value));
}

uint64_t rdmsr(uint32_t msrid) {
  uint32_t low;
  uint32_t high;
  __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msrid));
  return (uint64_t)low << 0 | (uint64_t)high << 32;
}

uint64_t wrmsr(uint32_t msrid, uint64_t value) {
  uint32_t low = value >> 0 & 0xFFFFFFFF;
  uint32_t high = value >> 32 & 0xFFFFFFFF;
  __asm__ __volatile__("wrmsr" : : "a"(low), "d"(high), "c"(msrid) : "memory");
  return value;
}

bool checkSSE() {
  uint32_t eax = 0x1, ebx = 0, ecx = 0, edx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  return (edx >> 25) & 1;
}

bool checkFXSR() {
  uint32_t eax = 0x1, ebx = 0, ecx = 0, edx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  return (edx >> 24) & 1;
}

extern void asm_enable_SSE();
void        initiateSSE() {
  if (!checkSSE()) {
    debugf("[sse] FATAL! No support for Streaming Simd Extensions found!\n");
    panic();
  }

  if (!checkFXSR()) {
    debugf("[sse] FATAL! No support for FXSR found!\n");
    panic();
  }

  // asm_enable_SSE();
  // enable SSE
  asm volatile("mov %%cr0, %%rax;"
                      "and $0xFFFB, %%ax;"
                      "or  $2, %%eax;"
                      "mov %%rax, %%cr0;"
                      "mov %%cr4, %%rax;"
                      "or  $0b11000000000, %%rax;"
                      "mov %%rax, %%cr4;"
               :
               :
               : "rax");

  // set NE in cr0 and reset x87 fpu
  asm volatile("fninit;"
                      "mov %%cr0, %%rax;"
                      "or $0b100000, %%rax;"
                      "mov %%rax, %%cr0;"
               :
               :
               : "rax");

  // Do AVX stuff under certain conditions (qemu and prolly other stuff fake
  // not having support but all AVX instructions succeed)
  uint32_t eax = 1, ebx = 0, ecx = 0, edx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);

  if (ecx & (1 << 26)) {
    debugf("[cpu] The xsave instruction is available. Enabling..\n");
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (uint32_t)1 << 18;
    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    if (ecx & (1 << 28)) {
      debugf("[cpu] The AVX extensions are available. Enabling..\n");
      uint32_t xcr0_lo = 0x7; // x87 (bit 0), SSE (bit 1), AVX (bit 2)
      uint32_t xcr0_hi = 0;
      asm volatile("xsetbv" ::"c"(0), "a"(xcr0_lo), "d"(xcr0_hi));
    }
  }

  debugf("[cpu] Extra CPU features have all been enabled without issue\n");
}

void panic() {
  debugf("[kernel] Kernel panic triggered!\n");
  asm volatile("cli");
  while (true)
    asm volatile("hlt");
}

void _assert(bool expression, char *file, int line) {
  if (!expression) {
    debugf("[assert] Assertation failed! file{%s:%d}\n", file, line);
    panic();
  }
}

bool checkInterrupts() {
  uint16_t flags;
  asm volatile("pushf; pop %0" : "=g"(flags));
  return flags & (1 << 9);
}

bool isProtectedMemory(uint64_t virt) {
  if (virt > bootloader.hhdmOffset &&
      virt < (bootloader.hhdmOffset + bootloader.mmTotal))
    return true;
  else if (virt > bootloader.kernelVirtBase &&
           virt < bootloader.kernelVirtBase + 268435456) {
    return true;
  }

  return false;
}

#include <paging.h>
#include <task.h>
void handControl() {
  if (!tasksInitiated)
    return;
  assert(checkInterrupts()); // no volatile contexts
  currentTask->schedPageFault = true;
  volatile uint8_t _drop = *(uint8_t *)(SCHED_PAGE_FAULT_MAGIC_ADDRESS);
  (void)(_drop);
}

// Endianness
uint16_t switch_endian_16(uint16_t val) { return (val << 8) | (val >> 8); }

uint32_t switch_endian_32(uint32_t val) {
  return (val << 24) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) |
         (val >> 24);
}
