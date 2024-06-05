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

// termios stuff
#define TCGETS 0x5401
#define TCSETS 0x5402

#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

#define IGNBRK 0000001
#define BRKINT 0000002
#define IGNPAR 0000004
#define PARMRK 0000010
#define INPCK 0000020
#define ISTRIP 0000040
#define INLCR 0000100
#define IGNCR 0000200
#define ICRNL 0000400
#define IUCLC 0001000
#define IXON 0002000
#define IXANY 0004000
#define IXOFF 0010000
#define IMAXBEL 0020000
#define IUTF8 0040000

#define OPOST 0000001
#define OLCUC 0000002
#define ONLCR 0000004
#define OCRNL 0000010
#define ONOCR 0000020
#define ONLRET 0000040
#define OFILL 0000100
#define OFDEL 0000200
#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE)
#define NLDLY 0000400
#define NL0 0000000
#define NL1 0000400
#define CRDLY 0003000
#define CR0 0000000
#define CR1 0001000
#define CR2 0002000
#define CR3 0003000
#define TABDLY 0014000
#define TAB0 0000000
#define TAB1 0004000
#define TAB2 0010000
#define TAB3 0014000
#define BSDLY 0020000
#define BS0 0000000
#define BS1 0020000
#define FFDLY 0100000
#define FF0 0000000
#define FF1 0100000
#endif

#define VTDLY 0040000
#define VT0 0000000
#define VT1 0040000

#define B0 0000000
#define B50 0000001
#define B75 0000002
#define B110 0000003
#define B134 0000004
#define B150 0000005
#define B200 0000006
#define B300 0000007
#define B600 0000010
#define B1200 0000011
#define B1800 0000012
#define B2400 0000013
#define B4800 0000014
#define B9600 0000015
#define B19200 0000016
#define B38400 0000017

#define B57600 0010001
#define B115200 0010002
#define B230400 0010003
#define B460800 0010004
#define B500000 0010005
#define B576000 0010006
#define B921600 0010007
#define B1000000 0010010
#define B1152000 0010011
#define B1500000 0010012
#define B2000000 0010013
#define B2500000 0010014
#define B3000000 0010015
#define B3500000 0010016
#define B4000000 0010017

#define CSIZE 0000060
#define CS5 0000000
#define CS6 0000020
#define CS7 0000040
#define CS8 0000060
#define CSTOPB 0000100
#define CREAD 0000200
#define PARENB 0000400
#define PARODD 0001000
#define HUPCL 0002000
#define CLOCAL 0004000

#define ISIG 0000001
#define ICANON 0000002
#define ECHO 0000010
#define ECHOE 0000020
#define ECHOK 0000040
#define ECHONL 0000100
#define NOFLSH 0000200
#define TOSTOP 0000400
#define IEXTEN 0100000

// maps
#define PROT_READ 0x1  /* Page can be read.  */
#define PROT_WRITE 0x2 /* Page can be written.  */
#define PROT_EXEC 0x4  /* Page can be executed.  */
#define PROT_NONE 0x0  /* Page can not be accessed.  */
#define PROT_GROWSDOWN                                                         \
  0x01000000 /* Extend change to start of                                      \
                growsdown vma (mprotect only).  */
#define PROT_GROWSUP                                                           \
  0x02000000 /* Extend change to start of                                      \
                growsup vma (mprotect only).  */

/* Sharing types (must choose one and only one of these).  */
#define MAP_SHARED 0x01  /* Share changes.  */
#define MAP_PRIVATE 0x02 /* Changes are private.  */
#define MAP_SHARED_VALIDATE                                                    \
  0x03                /* Share changes and validate                            \
                         extension flags.  */
#define MAP_TYPE 0x0f /* Mask for type of mapping.  */

/* Other flags.  */
#define MAP_FIXED 0x10 /* Interpret addr exactly.  */
#define MAP_FILE 0
#ifdef __MAP_ANONYMOUS
#define MAP_ANONYMOUS __MAP_ANONYMOUS /* Don't use a file.  */
#else
#define MAP_ANONYMOUS 0x20 /* Don't use a file.  */
#endif
#define MAP_ANON MAP_ANONYMOUS
/* When MAP_HUGETLB is set bits [26:31] encode the log2 of the huge page size.
 */
#define MAP_HUGE_SHIFT 26
#define MAP_HUGE_MASK 0x3f

/* Flags to `msync'.  */
#define MS_ASYNC 1      /* Sync memory asynchronously.  */
#define MS_SYNC 4       /* Synchronous memory sync.  */
#define MS_INVALIDATE 2 /* Invalidate the caches.  */

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
