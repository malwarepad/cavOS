#include <system.h>
#include <backupconsole.h>
#include <console.h>
#include <fat32.h>

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

void printfch(int character) {
  if (fat->works == 1)
    drawCharacter(character);
  else
    preFSconsole(character);
}
