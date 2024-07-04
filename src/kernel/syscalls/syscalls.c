#include <console.h>
#include <fb.h>
#include <gdt.h>
#include <isr.h>
#include <kb.h>
#include <schedule.h>
#include <serial.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

#if DEBUG_SYSCALLS_STRACE
#include <linux_syscalls.h>
#endif

// System call entry and management-related functions
// Copyright (C) 2024 Panagiotis

size_t   syscalls[MAX_SYSCALLS] = {0};
uint32_t syscallCnt = 0;

void registerSyscall(uint32_t id, void *handler) {
  if (id > MAX_SYSCALLS) {
    debugf("[syscalls] FATAL! Exceded limit! limit{%d} id{%d}\n", MAX_SYSCALLS,
           id);
    panic();
  }

  if (syscalls[id]) {
    debugf("[syscalls] FATAL! id{%d} found duplicate!\n", id);
    panic();
  }

  syscalls[id] = (size_t)handler;
  syscallCnt++;
}

typedef uint64_t (*SyscallHandler)(uint64_t a1, uint64_t a2, uint64_t a3,
                                   uint64_t a4, uint64_t a5, uint64_t a6);
void syscallHandler(AsmPassedInterrupt *regs) {
  uint64_t *rspPtr = (uint64_t *)((size_t)regs + sizeof(AsmPassedInterrupt));
  uint64_t  rsp = *rspPtr;

  currentTask->systemCallInProgress = true;
  currentTask->syscallRegs = regs;
  currentTask->syscallRsp = rsp;

  asm volatile("sti"); // do other task stuff while we're here!

  uint64_t id = regs->rax;
  if (id > MAX_SYSCALLS) {
    regs->rax = -1;
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls] FAIL! Tried to access syscall{%d} (out of bounds)!\n",
           id);
#endif
    goto cleanup;
  }
  size_t handler = syscalls[id];

#if DEBUG_SYSCALLS_STRACE
  // debugf("[syscalls] id{%d} handler{%lx}\n", id, handler);

  bool usable = id < (sizeof(linux_syscalls) / sizeof(linux_syscalls[0]));
  const LINUX_SYSCALL *info = &linux_syscalls[id];

  if (!handler)
    debugf("\033[0;31m");
  debugf("%d [syscalls] %s( ", currentTask->id, usable ? info->name : "???");
  if (usable) {
    if (info->rdi[0])
      debugf("\b%s:%lx ", info->rdi, regs->rdi);
    if (info->rsi[0])
      debugf("%s:%lx ", info->rsi, regs->rsi);
    if (info->rdx[0])
      debugf("%s:%lx ", info->rdx, regs->rdx);
    if (info->r10[0])
      debugf("%s:%lx ", info->r10, regs->r10);
    if (info->r8[0])
      debugf("%s:%lx ", info->r8, regs->r8);
    if (info->r9[0])
      debugf("%s:%lx ", info->r9, regs->r9);
  }
  debugf("\b)");
  if (!handler)
    debugf("\033[0m\n");
#endif

  if (!handler) {
    regs->rax = -ENOSYS;
#if DEBUG_SYSCALLS_MISSING
    debugf("[syscalls] Tried to access syscall{%d} (doesn't exist)!\n", id);
#endif
    goto cleanup;
  }

  long int ret = ((SyscallHandler)(handler))(regs->rdi, regs->rsi, regs->rdx,
                                             regs->r10, regs->r8, regs->r9);
#if DEBUG_SYSCALLS_STRACE
  debugf(" = %d\n", ret);
#endif

  regs->rax = ret;

cleanup:
  currentTask->syscallRsp = 0;
  currentTask->syscallRegs = 0;
  currentTask->systemCallInProgress = false;
}

// System calls themselves

void initiateSyscalls() {
  /*
   * Linux system call ranges
   * Handlers are defined in every file of linux/
   */

  // Filesystem operations
  syscallRegFs();

  // Memory management
  syscallRegMem();

  // POSIX signals
  syscallRegSig();

  // Task/Process environment
  syscallsRegEnv();

  // Task/Process management
  syscallsRegProc();

  // Time/Date/Clocks!
  syscallsRegClock();

  debugf("[syscalls] System calls are ready to fire: %d/%d\n", syscallCnt,
         MAX_SYSCALLS);
}
