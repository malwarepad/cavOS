#include "isr.h"
#include "multiboot2.h"
#include "types.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

#define MAX_SYSCALLS 420
uint32_t syscalls[MAX_SYSCALLS];

void registerSyscall(uint32_t id, void *handler);
void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();

#endif
