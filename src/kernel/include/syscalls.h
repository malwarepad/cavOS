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

/* Signal Debugging: None */
#define NO_DEBUG_SIGNALS 1

#if !NO_DEBUG_SIGNALS
#define DEBUG_SIGNALS_HITS 1
#define DEBUG_SIGNALS_STUB 1
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
void syscallsRegPoll();

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

char *atResolvePathname(int dirfd, char *pathname);
void  atResolvePathnameCleanup(char *pathname, char *resolved);

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

/* Event FDs (defined in eventfd.c) */
VfsHandlers eventFdHandlers;

size_t eventFdOpen(uint64_t initValue, int flags);

#define RET_IS_ERR(syscall_return) ((syscall_return) > -4096UL)

/* Signal stuff */
typedef enum SignalInternal {
  SIGNAL_INTERNAL_CORE = 0,
  SIGNAL_INTERNAL_TERM,
  SIGNAL_INTERNAL_IGN,
  SIGNAL_INTERNAL_STOP,
  SIGNAL_INTERNAL_CONT
} SignalInternal;

void   initiateSignalDefs();
void   signalsPendingHandleSys(void *taskPtr, uint64_t *rsp,
                               AsmPassedInterrupt *registers);
void   signalsPendingHandleSched(void *taskPtr);
size_t signalsSigreturnSyscall(void *taskPtr);
bool   signalsPendingQuick(void *taskPtr);
bool   signalsRevivableState(int state);

#if DEBUG_SIGNALS_HITS
#define dbgSigHitf debugf
#else
#define dbgSigHitf(...) ((void)0)
#endif
#if DEBUG_SIGNALS_STUB
#define dbgSigStubf debugf
#else
#define dbgSigStubf(...) ((void)0)
#endif

/* Cmon now it's really useful! */
size_t syscallRtSigprocmask(int how, sigset_t *nset, sigset_t *oset,
                            size_t sigsetsize);

#endif
