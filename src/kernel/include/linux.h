#include "types.h"

#ifndef LINUX_DEF_H
#define LINUX_DEF_H

// imagination
typedef int32_t   pid_t;
typedef int32_t   uid_t;
typedef int32_t   sigval_t;
typedef uint32_t  timer_t;
typedef uint32_t  mqd_t;
typedef uintptr_t address_t;

// /usr/include/bits/types/struct_iovec.h
typedef struct iovec {
  void  *iov_base; /* Pointer to data.  */
  size_t iov_len;  /* Length of data.  */
} iovec;

// /usr/include/bits/ioctl-types.h
typedef struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
} winsize;

// /usr/include/bits/types/__sigval_t.h
union __sigval {
  int   __sival_int;
  void *__sival_ptr;
};
typedef union __sigval __sigval_t;

// /usr/include/asm-generic/siginfo.h
typedef struct {
  int32_t si_signo; // Signal number
  int32_t si_errno; // Error number (if applicable)
  int32_t si_code;  // Signal code

  union {
    int32_t _pad[128 - 3 * sizeof(int32_t) / sizeof(int32_t)];

    // Kill
    struct {
      pid_t si_pid; // Sending process ID
      uid_t si_uid; // Real user ID of sending process
    } _kill;

    // Timer
    struct {
      int32_t  si_tid;     // Timer ID
      int32_t  si_overrun; // Overrun count
      sigval_t si_sigval;  // Signal value
    } _timer;

    // POSIX.1b signals
    struct {
      pid_t    si_pid;    // Sending process ID
      uid_t    si_uid;    // Real user ID of sending process
      sigval_t si_sigval; // Signal value
    } _rt;

    // SIGCHLD
    struct {
      pid_t   si_pid;    // Sending process ID
      uid_t   si_uid;    // Real user ID of sending process
      int32_t si_status; // Exit value or signal
      int32_t si_utime;  // User time consumed
      int32_t si_stime;  // System time consumed
    } _sigchld;

    // SIGILL, SIGFPE, SIGSEGV, SIGBUS
    struct {
      address_t si_addr;     // Faulting instruction or data address
      int32_t   si_addr_lsb; // LSB of the address (if applicable)
    } _sigfault;

    // SIGPOLL
    struct {
      int32_t si_band; // Band event
      int32_t si_fd;   // File descriptor
    } _sigpoll;

    // SIGSYS
    struct {
      address_t si_call_addr; // Calling user insn
      int32_t   si_syscall;   // Number of syscall
      uint32_t  si_arch;      // Architecture
    } _sigsys;
  } _sifields;
} siginfo_t;

// /usr/include/bits/types/__sigset_t.h
#define _SIGSET_NWORDS (1024 / (8 * sizeof(unsigned long int)))
typedef struct {
  unsigned long int __val[_SIGSET_NWORDS];
} __sigset_t;

// /usr/include/linux/time.h
typedef struct timespec {
  int64_t tv_sec;  // seconds
  int64_t tv_nsec; // nanoseconds
} timespec;

typedef struct timeval {
  int64_t tv_sec;  /* Seconds */
  int64_t tv_usec; /* Microseconds */
} timeval;

// /usr/include/bits/types/struct_rusage.h
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

// /usr/include/bits/sigaction.h
struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  } __sa_handler;
  __sigset_t sa_mask;
  int        sa_flags;
  void (*sa_restorer)(void);
};

// /usr/include/asm-generic/ioctls.h
#define TCGETS 0x5401
#define TCSETS 0x5402

// /usr/include/bits/termios-c_cc.h
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

// /usr/include/bits/termios-c_iflag.h
/* c_iflag bits */
#define IGNBRK 0000001 /* Ignore break condition.  */
#define BRKINT 0000002 /* Signal interrupt on break.  */
#define IGNPAR 0000004 /* Ignore characters with parity errors.  */
#define PARMRK 0000010 /* Mark parity and framing errors.  */
#define INPCK 0000020  /* Enable input parity check.  */
#define ISTRIP 0000040 /* Strip 8th bit off characters.  */
#define INLCR 0000100  /* Map NL to CR on input.  */
#define IGNCR 0000200  /* Ignore CR.  */
#define ICRNL 0000400  /* Map CR to NL on input.  */
#define IUCLC                                                                  \
  0001000             /* Map uppercase characters to lowercase on input        \
                         (not in POSIX).  */
#define IXON 0002000  /* Enable start/stop output control.  */
#define IXANY 0004000 /* Enable any character to restart output.  */
#define IXOFF 0010000 /* Enable start/stop input control.  */
#define IMAXBEL                                                                \
  0020000             /* Ring bell when input queue is full                    \
                         (not in POSIX).  */
#define IUTF8 0040000 /* Input is UTF8 (not in POSIX).  */

// /usr/include/bits/termios-c_oflag.h
/* c_oflag bits */
#define OPOST 0000001 /* Post-process output.  */
#define OLCUC                                                                  \
  0000002              /* Map lowercase characters to uppercase on output.     \
                          (not in POSIX).  */
#define ONLCR 0000004  /* Map NL to CR-NL on output.  */
#define OCRNL 0000010  /* Map CR to NL on output.  */
#define ONOCR 0000020  /* No CR output at column 0.  */
#define ONLRET 0000040 /* NL performs CR function.  */
#define OFILL 0000100  /* Use fill characters for delay.  */
#define OFDEL 0000200  /* Fill is DEL.  */
#if defined __USE_MISC || defined __USE_XOPEN
#define NLDLY 0000400  /* Select newline delays:  */
#define NL0 0000000    /* Newline type 0.  */
#define NL1 0000400    /* Newline type 1.  */
#define CRDLY 0003000  /* Select carriage-return delays:  */
#define CR0 0000000    /* Carriage-return delay type 0.  */
#define CR1 0001000    /* Carriage-return delay type 1.  */
#define CR2 0002000    /* Carriage-return delay type 2.  */
#define CR3 0003000    /* Carriage-return delay type 3.  */
#define TABDLY 0014000 /* Select horizontal-tab delays:  */
#define TAB0 0000000   /* Horizontal-tab delay type 0.  */
#define TAB1 0004000   /* Horizontal-tab delay type 1.  */
#define TAB2 0010000   /* Horizontal-tab delay type 2.  */
#define TAB3 0014000   /* Expand tabs to spaces.  */
#define BSDLY 0020000  /* Select backspace delays:  */
#define BS0 0000000    /* Backspace-delay type 0.  */
#define BS1 0020000    /* Backspace-delay type 1.  */
#define FFDLY 0100000  /* Select form-feed delays:  */
#define FF0 0000000    /* Form-feed delay type 0.  */
#define FF1 0100000    /* Form-feed delay type 1.  */
#endif

#define VTDLY 0040000 /* Select vertical-tab delays:  */
#define VT0 0000000   /* Vertical-tab delay type 0.  */
#define VT1 0040000   /* Vertical-tab delay type 1.  */

// /usr/include/bits/termios.h
/* c_cflag bit meaning */
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

/* Extra output baud rates (not in POSIX).  */
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

// /usr/include/bits/termios-c_cflag.h
/* c_cflag bits.  */
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

// /usr/include/bits/termios-c_lflag.h
/* c_lflag bits */
#define ISIG 0000001   /* Enable signals.  */
#define ICANON 0000002 /* Canonical input (erase and kill processing).  */
#if defined __USE_MISC || (defined __USE_XOPEN && !defined __USE_XOPEN2K)
#define XCASE 0000004
#endif
#define ECHO 0000010 /* Enable echo.  */
#define ECHOE                                                                  \
  0000020              /* Echo erase character as error-correcting             \
                          backspace.  */
#define ECHOK 0000040  /* Echo KILL.  */
#define ECHONL 0000100 /* Echo NL.  */
#define NOFLSH 0000200 /* Disable flush after interrupt or quit.  */
#define TOSTOP 0000400 /* Send SIGTTOU for background output.  */
#ifdef __USE_MISC
#define ECHOCTL                                                                \
  0001000 /* If ECHO is also set, terminal special characters                  \
             other than TAB, NL, START, and STOP are echoed as                 \
             ^X, where X is the character with ASCII code 0x40                 \
             greater than the special character                                \
             (not in POSIX).  */
#define ECHOPRT                                                                \
  0002000 /* If ICANON and ECHO are also set, characters are                   \
             printed as they are being erased                                  \
             (not in POSIX).  */
#define ECHOKE                                                                 \
  0004000 /* If ICANON is also set, KILL is echoed by erasing                  \
             each character on the line, as specified by ECHOE                 \
             and ECHOPRT (not in POSIX).  */
#define FLUSHO                                                                 \
  0010000 /* Output is being flushed.  This flag is toggled by                 \
             typing the DISCARD character (not in POSIX).  */
#define PENDIN                                                                 \
  0040000 /* All characters in the input queue are reprinted                   \
             when the next character is read                                   \
             (not in POSIX).  */
#endif
#define IEXTEN                                                                 \
  0100000 /* Enable implementation-defined input                               \
             processing.  */
#ifdef __USE_MISC
#define EXTPROC 0200000
#endif

// /usr/include/bits/mman-linux.h
/* Protections are chosen from these bits, OR'd together.  The
   implementation does not necessarily support PROT_EXEC or PROT_WRITE
   without PROT_READ.  The only guarantees are that no writing will be
   allowed without PROT_WRITE and no access will be allowed for PROT_NONE. */
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

// /usr/include/linux/time.h
// Standard POSIX clocks
#define CLOCK_REALTIME                                                         \
  0 // System-wide clock that measures real (wall-clock) time
#define CLOCK_MONOTONIC                                                        \
  1 // Monotonic time since some unspecified starting point
#define CLOCK_PROCESS_CPUTIME_ID                                               \
  2 // High-resolution per-process timer from the CPU
#define CLOCK_THREAD_CPUTIME_ID                                                \
  3 // High-resolution per-thread timer from the CPU

// Linux-specific clocks
#define CLOCK_MONOTONIC_RAW 4 // Monotonic time not subject to NTP adjustments
#define CLOCK_REALTIME_COARSE                                                  \
  5 // Faster but less precise version of CLOCK_REALTIME
#define CLOCK_MONOTONIC_COARSE                                                 \
  6                      // Faster but less precise version of CLOCK_MONOTONIC
#define CLOCK_BOOTTIME 7 // Monotonic time since boot, including suspend time
#define CLOCK_BOOTTIME_ALARM                                                   \
  8 // Like CLOCK_BOOTTIME but can wake system from suspend
#define CLOCK_TAI                                                              \
  9 // International Atomic Time (TAI) clock, not subject to leap seconds

#endif
