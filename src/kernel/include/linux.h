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

// /usr/include/x86_64-linux-gnu/bits/typesizes.h
#define __SYSCALL_SLONG_TYPE __SLONGWORD_TYPE
#define __SYSCALL_ULONG_TYPE __ULONGWORD_TYPE

#define __DEV_T_TYPE __UQUAD_TYPE
#define __UID_T_TYPE __U32_TYPE
#define __GID_T_TYPE __U32_TYPE
#define __INO_T_TYPE __SYSCALL_ULONG_TYPE
#define __INO64_T_TYPE __UQUAD_TYPE
#define __MODE_T_TYPE __U32_TYPE

#define __NLINK_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSWORD_T_TYPE __SYSCALL_SLONG_TYPE

#define __OFF_T_TYPE __SYSCALL_SLONG_TYPE
#define __OFF64_T_TYPE __SQUAD_TYPE
#define __PID_T_TYPE __S32_TYPE
#define __RLIM_T_TYPE __SYSCALL_ULONG_TYPE
#define __RLIM64_T_TYPE __UQUAD_TYPE
#define __BLKCNT_T_TYPE __SYSCALL_SLONG_TYPE
#define __BLKCNT64_T_TYPE __SQUAD_TYPE
#define __FSBLKCNT_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSBLKCNT64_T_TYPE __UQUAD_TYPE
#define __FSFILCNT_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSFILCNT64_T_TYPE __UQUAD_TYPE
#define __ID_T_TYPE __U32_TYPE
#define __CLOCK_T_TYPE __SYSCALL_SLONG_TYPE
#define __TIME_T_TYPE __SYSCALL_SLONG_TYPE
#define __USECONDS_T_TYPE __U32_TYPE
#define __SUSECONDS_T_TYPE __SYSCALL_SLONG_TYPE
#define __SUSECONDS64_T_TYPE __SQUAD_TYPE
#define __DADDR_T_TYPE __S32_TYPE
#define __KEY_T_TYPE __S32_TYPE
#define __CLOCKID_T_TYPE __S32_TYPE
#define __TIMER_T_TYPE void *
#define __BLKSIZE_T_TYPE __SYSCALL_SLONG_TYPE
#define __FSID_T_TYPE                                                          \
  struct {                                                                     \
    int __val[2];                                                              \
  }
#define __SSIZE_T_TYPE __SWORD_TYPE
#define __CPU_MASK_TYPE __SYSCALL_ULONG_TYPE

// /usr/include/x86_64-linux-gnu/bits/types.h
#define __S16_TYPE short int
#define __U16_TYPE unsigned short int
#define __S32_TYPE int
#define __U32_TYPE unsigned int
#define __SLONGWORD_TYPE long int
#define __ULONGWORD_TYPE unsigned long int

#define __SQUAD_TYPE long int
#define __UQUAD_TYPE unsigned long int
#define __SWORD_TYPE long int
#define __UWORD_TYPE unsigned long int
#define __SLONG32_TYPE int
#define __ULONG32_TYPE unsigned int
#define __S64_TYPE long int
#define __U64_TYPE unsigned long int
/* No need to mark the typedef with __extension__.   */
#define __STD_TYPE typedef

__STD_TYPE __DEV_T_TYPE   __dev_t;   /* Type of device numbers.  */
__STD_TYPE __UID_T_TYPE   __uid_t;   /* Type of user identifications.  */
__STD_TYPE __GID_T_TYPE   __gid_t;   /* Type of group identifications.  */
__STD_TYPE __INO_T_TYPE   __ino_t;   /* Type of file serial numbers.  */
__STD_TYPE __INO64_T_TYPE __ino64_t; /* Type of file serial numbers (LFS).*/
__STD_TYPE __MODE_T_TYPE  __mode_t;  /* Type of file attribute bitmasks.  */
__STD_TYPE __NLINK_T_TYPE __nlink_t; /* Type of file link counts.  */
__STD_TYPE __OFF_T_TYPE   __off_t;   /* Type of file sizes and offsets.  */
__STD_TYPE __OFF64_T_TYPE __off64_t; /* Type of file sizes and offsets (LFS). */
__STD_TYPE __PID_T_TYPE   __pid_t;   /* Type of process identifications.  */
__STD_TYPE __FSID_T_TYPE  __fsid_t;  /* Type of file system IDs.  */
__STD_TYPE __CLOCK_T_TYPE __clock_t; /* Type of CPU usage counts.  */
__STD_TYPE __RLIM_T_TYPE  __rlim_t;  /* Type for resource measurement.  */
__STD_TYPE __RLIM64_T_TYPE
                         __rlim64_t; /* Type for resource measurement (LFS).  */
__STD_TYPE __ID_T_TYPE   __id_t;     /* General type for IDs.  */
__STD_TYPE __TIME_T_TYPE __time_t;   /* Seconds since the Epoch.  */
__STD_TYPE __USECONDS_T_TYPE  __useconds_t;  /* Count of microseconds.  */
__STD_TYPE __SUSECONDS_T_TYPE __suseconds_t; /* Signed count of microseconds. */
__STD_TYPE __SUSECONDS64_T_TYPE __suseconds64_t;

__STD_TYPE __DADDR_T_TYPE __daddr_t; /* The type of a disk address.  */
__STD_TYPE __KEY_T_TYPE   __key_t;   /* Type of an IPC key.  */

/* Clock ID used in clock and timer functions.  */
__STD_TYPE __CLOCKID_T_TYPE __clockid_t;

/* Timer ID returned by `timer_create'.  */
__STD_TYPE __TIMER_T_TYPE __timer_t;

/* Type to represent block size.  */
__STD_TYPE __BLKSIZE_T_TYPE __blksize_t;

/* Types from the Large File Support interface.  */

/* Type to count number of disk blocks.  */
__STD_TYPE __BLKCNT_T_TYPE   __blkcnt_t;
__STD_TYPE __BLKCNT64_T_TYPE __blkcnt64_t;

/* Type to count file system blocks.  */
__STD_TYPE __FSBLKCNT_T_TYPE   __fsblkcnt_t;
__STD_TYPE __FSBLKCNT64_T_TYPE __fsblkcnt64_t;

/* Type to count file system nodes.  */
__STD_TYPE __FSFILCNT_T_TYPE   __fsfilcnt_t;
__STD_TYPE __FSFILCNT64_T_TYPE __fsfilcnt64_t;

/* Type of miscellaneous file system fields.  */
__STD_TYPE __FSWORD_T_TYPE __fsword_t;

__STD_TYPE __SSIZE_T_TYPE __ssize_t; /* Type of a byte count, or error.  */

/* Signed long type used in system calls.  */
__STD_TYPE __SYSCALL_SLONG_TYPE __syscall_slong_t;
/* Unsigned long type used in system calls.  */
__STD_TYPE __SYSCALL_ULONG_TYPE __syscall_ulong_t;

/* These few don't really vary by system, they always correspond
   to one of the other defined types.  */
typedef __off64_t __loff_t; /* Type of file sizes and offsets (LFS).  */
typedef char     *__caddr_t;

/* Duplicates info from stdint.h but this is used in unistd.h.  */
__STD_TYPE __SWORD_TYPE __intptr_t;

/* Duplicate info from sys/socket.h.  */
__STD_TYPE __U32_TYPE __socklen_t;

/* C99: An integer type that can be accessed as an atomic entity,
   even in the presence of asynchronous interrupts.
   It is not currently necessary for this to be machine-specific.  */
typedef int __sig_atomic_t;

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
#define TCSETSW 0x5403

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

// https://docs.huihoo.com/doxygen/linux/kernel/3.7/uapi_2linux_2utsname_8h_source.html
struct old_utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
};

// /usr/include/sys/wait.h
#define WNOHANG 1
#define WUNTRACED 2

#define WSTOPPED 2
#define WEXITED 4
#define WCONTINUED 8
#define WNOWAIT 0x1000000

// /usr/include/linux/stat.h
#define S_IFMT 00170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

// /usr/include/x86_64-linux-gnu/bits/struct_stat.h
typedef struct stat {
#ifdef __USE_TIME_BITS64
#include <bits/struct_stat_time64_helper.h>
#else
  __dev_t st_dev; /* Device.  */
#ifndef __x86_64__
  unsigned short int __pad1;
#endif
#if defined __x86_64__ || !defined __USE_FILE_OFFSET64
  __ino_t st_ino; /* File serial number.	*/
#else
  __ino_t __st_ino; /* 32bit file serial number.	*/
#endif
#ifndef __x86_64__
  __mode_t  st_mode;  /* File mode.  */
  __nlink_t st_nlink; /* Link count.  */
#else
  __nlink_t st_nlink; /* Link count.  */
  __mode_t  st_mode;  /* File mode.  */
#endif
  __uid_t st_uid; /* User ID of the file's owner.	*/
  __gid_t st_gid; /* Group ID of the file's group.*/
#ifdef __x86_64__
  int __pad0;
#endif
  __dev_t st_rdev; /* Device number, if device.  */
#ifndef __x86_64__
  unsigned short int __pad2;
#endif
#if defined __x86_64__ || !defined __USE_FILE_OFFSET64
  __off_t st_size; /* Size of file, in bytes.  */
#else
  __off64_t st_size; /* Size of file, in bytes.  */
#endif
  __blksize_t st_blksize; /* Optimal block size for I/O.  */
#if defined __x86_64__ || !defined __USE_FILE_OFFSET64
  __blkcnt_t st_blocks; /* Number 512-byte blocks allocated. */
#else
  __blkcnt64_t st_blocks; /* Number 512-byte blocks allocated. */
#endif
#ifdef __USE_XOPEN2K8
  /* Nanosecond resolution timestamps are stored in a format
     equivalent to 'struct timespec'.  This is the type used
     whenever possible but the Unix namespace rules do not allow the
     identifier 'timespec' to appear in the <sys/stat.h> header.
     Therefore we have to handle the use of this header in strictly
     standard-compliant sources special.  */
  struct timespec st_atim; /* Time of last access.  */
  struct timespec st_mtim; /* Time of last modification.  */
  struct timespec st_ctim; /* Time of last status change.  */
#define st_atime st_atim.tv_sec /* Backward compatibility.  */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#else
  __time_t          st_atime;     /* Time of last access.  */
  __syscall_ulong_t st_atimensec; /* Nscecs of last access.  */
  __time_t          st_mtime;     /* Time of last modification.  */
  __syscall_ulong_t st_mtimensec; /* Nsecs of last modification.  */
  __time_t          st_ctime;     /* Time of last status change.  */
  __syscall_ulong_t st_ctimensec; /* Nsecs of last status change.  */
#endif
#ifdef __x86_64__
  __syscall_slong_t __glibc_reserved[3];
#else
#ifndef __USE_FILE_OFFSET64
  unsigned long int __glibc_reserved4;
  unsigned long int __glibc_reserved5;
#else
  __ino64_t st_ino; /* File serial number.	*/
#endif
#endif
#endif /* __USE_TIME_BITS64  */
} stat;

// https://linux.die.net/man/2/getdents64
struct linux_dirent64 {

  uint64_t       d_ino;
  int64_t        d_off;
  unsigned short d_reclen;
  unsigned char  d_type;
  char           d_name[];
  // unsigned long  d_ino;    /* Inode number */
  // unsigned long  d_off;    /* Offset to next linux_dirent */
  // unsigned short d_reclen; /* Length of this linux_dirent */
  // char           d_name[]; /* Filename (null-terminated) */
  /* length is actually (d_reclen - 2 -
     offsetof(struct linux_dirent, d_name) */
  /*
  char           pad;       // Zero padding byte
  char           d_type;    // File type (only since Linux 2.6.4;
                            // offset is (d_reclen - 1))
  */
};

#endif
