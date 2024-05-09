#include <fastSyscall.h>
#include <gdt.h>
#include <isr.h>
#include <system.h>

// Prepares the "syscall" instruction x86_64 provides
// Copyright (C) 2024 Panagiotis

bool checkSyscallInst() {
  uint32_t eax = 0x80000001, ebx = 0, ecx = 0, edx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  return (edx >> 11) & 1;
}

void initiateSyscallInst() {
  if (!checkSyscallInst()) {
    debugf("[syscalls] FATAL! No support for the syscall instruction found!\n");
    panic();
  }

  uint64_t star_reg = rdmsr(MSRID_STAR);
  star_reg &= 0x00000000ffffffff;

  star_reg |= ((uint64_t)GDT_USER_CODE - 16) << 48;
  star_reg |= ((uint64_t)GDT_KERNEL_CODE) << 32;
  wrmsr(MSRID_STAR, star_reg);

  // entry defined in isr.asm
  wrmsr(MSRID_LSTAR, (uint64_t)syscall_entry);

  uint64_t efer_reg = rdmsr(MSRID_EFER);
  efer_reg |= 1 << 0;
  wrmsr(MSRID_EFER, efer_reg);

  wrmsr(MSRID_FMASK, RFLAGS_IF | RFLAGS_DF);
}
