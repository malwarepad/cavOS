#include <backupconsole.h>
#include <console.h>
#include <fat32.h>
#include <system.h>

// Source code for handling ports via assembly references
// Copyright (C) 2023 Panagiotis

uint8 inportb(uint16 _port) {
  uint8 rv;
  __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "dN"(_port));
  return rv;
}

void outportb(uint16 _port, uint8 _data) {
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

void panic() {
  debugf("Kernel panic triggered!\n");
  asm("cli");
  asm("hlt");
}

bool checkInterrupts() {
  uint16_t flags;
  asm volatile("pushf; pop %0" : "=g"(flags));
  return flags & (1 << 9);
}

bool interruptStatus = true;

void lockInterrupts() {
  interruptStatus = checkInterrupts();
  asm("cli");
}

void releaseInterrupts() {
  if (interruptStatus)
    asm("sti");
  else
    interruptStatus = !interruptStatus;
}

void printfch(int character) {
  if (fat->works == 1)
    drawCharacter(character);
  else
    preFSconsole(character);
}

void putchar_(char c) { printfch(c); }

// Endianness
uint16_t switch_endian_16(uint16_t val) { return (val << 8) | (val >> 8); }

uint32_t switch_endian_32(uint32_t val) {
  return (val << 24) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) |
         (val >> 24);
}
