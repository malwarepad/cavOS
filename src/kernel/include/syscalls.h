#include "isr.h"
#include "types.h"
#include "vfs.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

#define MAX_SYSCALLS 450

/* Ignore everything! */

/* Syscall Debugging: None */
#define NO_DEBUG_SYSCALLS 1

#if !NO_DEBUG_SYSCALLS
/* Syscall Debugging: Comprehensive */
#define DEBUG_SYSCALLS_STRACE 1
#define DEBUG_SYSCALLS_EXTRA 1

/* Syscall Debugging: Important */
#define DEBUG_SYSCALLS_FAILS 1
#define DEBUG_SYSCALLS_STUB 1

/* Syscall Debugging: Essential */
#define DEBUG_SYSCALLS_MISSING 1
#endif

void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();

/* System call registration */
void syscallRegFs();
void syscallRegMem();
void syscallRegSig();
void syscallsRegEnv();
void syscallsRegProc();
void syscallsRegClock();
void syscallsRegNet();

void registerSyscall(uint32_t id, void *handler); // <- the master

/* Debugf helpers */
#if DEBUG_SYSCALLS_FAILS
int dbgSysFailf(const char *format, ...);
#else
#define dbgSysFailf(...) ((void)0)
#endif

#if DEBUG_SYSCALLS_EXTRA
int dbgSysExtraf(const char *format, ...);
#else
#define dbgSysExtraf(...) ((void)0)
#endif

#if DEBUG_SYSCALLS_STUB
int dbgSysStubf(const char *format, ...);
#else
#define dbgSysStubf(...) ((void)0)
#endif

/* Standard output handlers (io.c) */
size_t readHandler(OpenFile *fd, uint8_t *in, size_t limit);
size_t writeHandler(OpenFile *fd, uint8_t *out, size_t limit);
size_t ioctlHandler(OpenFile *fd, uint64_t request, void *arg);

size_t mmapHandler(size_t addr, size_t length, int prot, int flags,
                   OpenFile *fd, size_t pgoffset);

/* Defined in io.c */
extern VfsHandlers stdio;

/* Unix pipe() (defined in pipe.c) */
VfsHandlers pipeReadEnd;
VfsHandlers pipeWriteEnd;

bool   pipeCloseEnd(OpenFile *readFd);
size_t pipeOpen(int *fds);

#define RET_IS_ERR(syscall_return) ((syscall_return) > -4096UL)

#endif
