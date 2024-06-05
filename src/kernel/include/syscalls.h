#include "fs_controller.h"
#include "isr.h"
#include "types.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

#define MAX_SYSCALLS 420

/* Syscall Debugging: Comprehensive */
#define DEBUG_SYSCALLS 1
#define DEBUG_SYSCALLS_ARGS 1

/* Syscall Debugging: Important */
#define DEBUG_SYSCALLS_FAILS 1
#define DEBUG_SYSCALLS_STUB 1

/* Syscall Debugging: Essential */
#define DEBUG_SYSCALLS_MISSING 1

/* Linux's structures */
typedef struct iovec {
  void  *iov_base; /* Pointer to data.  */
  size_t iov_len;  /* Length of data.  */
} iovec;

typedef struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
} winsize;

union sigval {
  int   sival_int;
  void *sival_ptr;
};

typedef struct {
#ifdef __SI_SWAP_ERRNO_CODE
  int si_signo, si_code, si_errno;
#else
  int si_signo, si_errno, si_code;
#endif
  union {
    char __pad[128 - 2 * sizeof(int) - sizeof(long)];
    struct {
      union {
        struct {
          uint32_t si_pid;
          uint32_t si_uid;
        } __piduid;
        struct {
          int si_timerid;
          int si_overrun;
        } __timer;
      } __first;
      union {
        union sigval si_value;
        struct {
          int      si_status;
          long int si_utime, si_stime;
        } __sigchld;
      } __second;
    } __si_common;
    struct {
      void *si_addr;
      short si_addr_lsb;
      union {
        struct {
          void *si_lower;
          void *si_upper;
        } __addr_bnd;
        unsigned si_pkey;
      } __first;
    } __sigfault;
    struct {
      long si_band;
      int  si_fd;
    } __sigpoll;
    struct {
      void    *si_call_addr;
      int      si_syscall;
      unsigned si_arch;
    } __sigsys;
  } __si_fields;
} siginfo_t;

typedef struct __sigset_t {
  unsigned long __bits[128 / sizeof(long)];
} sigset_t;

typedef struct timespec {
  int64_t tv_sec;  // seconds
  int64_t tv_nsec; // nanoseconds
} timespec;

typedef struct timeval {
  int64_t tv_sec;  /* Seconds */
  int64_t tv_usec; /* Microseconds */
} timeval;

typedef struct rusage {
  timeval ru_utime;    /* user CPU time used */
  timeval ru_stime;    /* system CPU time used */
  long    ru_maxrss;   /* maximum resident set size */
  long    ru_ixrss;    /* integral shared memory size */
  long    ru_idrss;    /* integral unshared data size */
  long    ru_isrss;    /* integral unshared stack size */
  long    ru_minflt;   /* page reclaims (soft page faults) */
  long    ru_majflt;   /* page faults (hard page faults) */
  long    ru_nswap;    /* swaps */
  long    ru_inblock;  /* block input operations */
  long    ru_oublock;  /* block output operations */
  long    ru_msgsnd;   /* IPC messages sent */
  long    ru_msgrcv;   /* IPC messages received */
  long    ru_nsignals; /* signals received */
  long    ru_nvcsw;    /* voluntary context switches */
  long    ru_nivcsw;   /* involuntary context switches */
} rusage;

struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  } __sa_handler;
  sigset_t sa_mask;
  int      sa_flags;
  void (*sa_restorer)(void);
};
#define sa_handler __sa_handler.sa_handler
#define sa_sigaction __sa_handler.sa_sigaction

void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();

/* System call registration */
void syscallRegFs();
void syscallRegMem();
void syscallRegSig();
void syscallsRegEnv();
void syscallsRegProc();
void syscallsRegClock();

void registerSyscall(uint32_t id, void *handler); // <- the master

/* Standard output handlers (io.c) */
int readHandler(OpenFile *fd, uint8_t *in, size_t limit);
int writeHandler(OpenFile *fd, uint8_t *out, size_t limit);
int ioctlHandler(OpenFile *fd, uint64_t request, void *arg);

size_t mmapHandler(size_t addr, size_t length, int prot, int flags,
                   OpenFile *fd, size_t pgoffset);

/* Defined in io.c */
extern SpecialHandlers stdio;

#endif
