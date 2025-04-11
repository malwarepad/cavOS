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

// /usr/include/asm-generic/ioctls.h
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404

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
#if 0
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
  struct timespec st_atim;      /* Time of last access.  */
  struct timespec st_mtim;      /* Time of last modification.  */
  struct timespec st_ctim;      /* Time of last status change.  */
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

// include/linux/coda.h
/*
 * File types
 */
#define CDT_UNKNOWN 0
#define CDT_FIFO 1
#define CDT_CHR 2
#define CDT_DIR 4
#define CDT_BLK 6
#define CDT_REG 8
#define CDT_LNK 10
#define CDT_SOCK 12
#define CDT_WHT 14

// assumption
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;

typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

// include/linux/stat.h
/*
 * Timestamp structure for the timestamps in struct statx.
 *
 * tv_sec holds the number of seconds before (negative) or after (positive)
 * 00:00:00 1st January 1970 UTC.
 *
 * tv_nsec holds a number of nanoseconds (0..999,999,999) after the tv_sec time.
 *
 * __reserved is held in case we need a yet finer resolution.
 */
struct statx_timestamp {
  __s64 tv_sec;
  __u32 tv_nsec;
  __s32 __reserved;
};

struct statx {
  /* 0x00 */
  __u32 stx_mask;    /* What results were written [uncond] */
  __u32 stx_blksize; /* Preferred general I/O size [uncond] */
  __u64
      stx_attributes; /* Flags conveying information about the file [uncond] */
  /* 0x10 */
  __u32 stx_nlink; /* Number of hard links */
  __u32 stx_uid;   /* User ID of owner */
  __u32 stx_gid;   /* Group ID of owner */
  __u16 stx_mode;  /* File mode */
  __u16 __spare0[1];
  /* 0x20 */
  __u64 stx_ino;    /* Inode number */
  __u64 stx_size;   /* File size */
  __u64 stx_blocks; /* Number of 512-byte blocks allocated */
  __u64
      stx_attributes_mask; /* Mask to show what's supported in stx_attributes */
  /* 0x40 */
  struct statx_timestamp stx_atime; /* Last access time */
  struct statx_timestamp stx_btime; /* File creation time */
  struct statx_timestamp stx_ctime; /* Last attribute change time */
  struct statx_timestamp stx_mtime; /* Last data modification time */
  /* 0x80 */
  __u32 stx_rdev_major; /* Device ID of special file [if bdev/cdev] */
  __u32 stx_rdev_minor;
  __u32 stx_dev_major; /* ID of device containing file [uncond] */
  __u32 stx_dev_minor;
  /* 0x90 */
  __u64 stx_mnt_id;
  __u32 stx_dio_mem_align;    /* Memory buffer alignment for direct I/O */
  __u32 stx_dio_offset_align; /* File offset alignment for direct I/O */
  /* 0xa0 */
  __u64 __spare3[12]; /* Spare space for future expansion */
                      /* 0x100 */
};

#define STATX_TYPE 0x00000001U   /* Want/got stx_mode & S_IFMT */
#define STATX_MODE 0x00000002U   /* Want/got stx_mode & ~S_IFMT */
#define STATX_NLINK 0x00000004U  /* Want/got stx_nlink */
#define STATX_UID 0x00000008U    /* Want/got stx_uid */
#define STATX_GID 0x00000010U    /* Want/got stx_gid */
#define STATX_ATIME 0x00000020U  /* Want/got stx_atime */
#define STATX_MTIME 0x00000040U  /* Want/got stx_mtime */
#define STATX_CTIME 0x00000080U  /* Want/got stx_ctime */
#define STATX_INO 0x00000100U    /* Want/got stx_ino */
#define STATX_SIZE 0x00000200U   /* Want/got stx_size */
#define STATX_BLOCKS 0x00000400U /* Want/got stx_blocks */
#define STATX_BASIC_STATS                                                      \
  0x000007ffU                           /* The stuff in the normal stat struct \
                                         */
#define STATX_BTIME 0x00000800U         /* Want/got stx_btime */
#define STATX_MNT_ID 0x00001000U        /* Got stx_mnt_id */
#define STATX_DIOALIGN 0x00002000U      /* Want/got direct I/O alignment info */
#define STATX_MNT_ID_UNIQUE 0x00004000U /* Want/got extended stx_mount_id */

#define STATX__RESERVED                                                        \
  0x80000000U /* Reserved for future struct statx expansion */

// include/linux/fcntl.h
#define AT_FDCWD                                                               \
  -100 /* Special value used to indicate                                       \
          openat should use the current                                        \
          working directory. */

#define AT_SYMLINK_NOFOLLOW 0x100 /* Do not follow symbolic links.  */

// include/asm-generic/errno-base.h
#define EPERM 1    /* Operation not permitted */
#define ENOENT 2   /* No such file or directory */
#define ESRCH 3    /* No such process */
#define EINTR 4    /* Interrupted system call */
#define EIO 5      /* I/O error */
#define ENXIO 6    /* No such device or address */
#define E2BIG 7    /* Argument list too long */
#define ENOEXEC 8  /* Exec format error */
#define EBADF 9    /* Bad file number */
#define ECHILD 10  /* No child processes */
#define EAGAIN 11  /* Try again */
#define ENOMEM 12  /* Out of memory */
#define EACCES 13  /* Permission denied */
#define EFAULT 14  /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16   /* Device or resource busy */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Cross-device link */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* File table overflow */
#define EMFILE 24  /* Too many open files */
#define ENOTTY 25  /* Not a typewriter */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27   /* File too large */
#define ENOSPC 28  /* No space left on device */
#define ESPIPE 29  /* Illegal seek */
#define EROFS 30   /* Read-only file system */
#define EMLINK 31  /* Too many links */
#define EPIPE 32   /* Broken pipe */
#define EDOM 33    /* Math argument out of domain of func */
#define ERANGE 34  /* Math result not representable */

// include/asm-generic/errno.h
#define EDEADLK 35      /* Resource deadlock would occur */
#define ENAMETOOLONG 36 /* File name too long */
#define ENOLCK 37       /* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
#define ENOSYS 38 /* Invalid system call number */

#define ENOTEMPTY 39       /* Directory not empty */
#define ELOOP 40           /* Too many symbolic links encountered */
#define EWOULDBLOCK EAGAIN /* Operation would block */
#define ENOMSG 42          /* No message of desired type */
#define EIDRM 43           /* Identifier removed */
#define ECHRNG 44          /* Channel number out of range */
#define EL2NSYNC 45        /* Level 2 not synchronized */
#define EL3HLT 46          /* Level 3 halted */
#define EL3RST 47          /* Level 3 reset */
#define ELNRNG 48          /* Link number out of range */
#define EUNATCH 49         /* Protocol driver not attached */
#define ENOCSI 50          /* No CSI structure available */
#define EL2HLT 51          /* Level 2 halted */
#define EBADE 52           /* Invalid exchange */
#define EBADR 53           /* Invalid request descriptor */
#define EXFULL 54          /* Exchange full */
#define ENOANO 55          /* No anode */
#define EBADRQC 56         /* Invalid request code */
#define EBADSLT 57         /* Invalid slot */

#define EDEADLOCK EDEADLK

#define EBFONT 59          /* Bad font file format */
#define ENOSTR 60          /* Device not a stream */
#define ENODATA 61         /* No data available */
#define ETIME 62           /* Timer expired */
#define ENOSR 63           /* Out of streams resources */
#define ENONET 64          /* Machine is not on the network */
#define ENOPKG 65          /* Package not installed */
#define EREMOTE 66         /* Object is remote */
#define ENOLINK 67         /* Link has been severed */
#define EADV 68            /* Advertise error */
#define ESRMNT 69          /* Srmount error */
#define ECOMM 70           /* Communication error on send */
#define EPROTO 71          /* Protocol error */
#define EMULTIHOP 72       /* Multihop attempted */
#define EDOTDOT 73         /* RFS specific error */
#define EBADMSG 74         /* Not a data message */
#define EOVERFLOW 75       /* Value too large for defined data type */
#define ENOTUNIQ 76        /* Name not unique on network */
#define EBADFD 77          /* File descriptor in bad state */
#define EREMCHG 78         /* Remote address changed */
#define ELIBACC 79         /* Can not access a needed shared library */
#define ELIBBAD 80         /* Accessing a corrupted shared library */
#define ELIBSCN 81         /* .lib section in a.out corrupted */
#define ELIBMAX 82         /* Attempting to link in too many shared libraries */
#define ELIBEXEC 83        /* Cannot exec a shared library directly */
#define EILSEQ 84          /* Illegal byte sequence */
#define ERESTART 85        /* Interrupted system call should be restarted */
#define ESTRPIPE 86        /* Streams pipe error */
#define EUSERS 87          /* Too many users */
#define ENOTSOCK 88        /* Socket operation on non-socket */
#define EDESTADDRREQ 89    /* Destination address required */
#define EMSGSIZE 90        /* Message too long */
#define EPROTOTYPE 91      /* Protocol wrong type for socket */
#define ENOPROTOOPT 92     /* Protocol not available */
#define EPROTONOSUPPORT 93 /* Protocol not supported */
#define ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define EOPNOTSUPP 95      /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT 96    /* Protocol family not supported */
#define EAFNOSUPPORT 97    /* Address family not supported by protocol */
#define EADDRINUSE 98      /* Address already in use */
#define EADDRNOTAVAIL 99   /* Cannot assign requested address */
#define ENETDOWN 100       /* Network is down */
#define ENETUNREACH 101    /* Network is unreachable */
#define ENETRESET 102      /* Network dropped connection because of reset */
#define ECONNABORTED 103   /* Software caused connection abort */
#define ECONNRESET 104     /* Connection reset by peer */
#define ENOBUFS 105        /* No buffer space available */
#define EISCONN 106        /* Transport endpoint is already connected */
#define ENOTCONN 107       /* Transport endpoint is not connected */
#define ESHUTDOWN 108      /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS 109   /* Too many references: cannot splice */
#define ETIMEDOUT 110      /* Connection timed out */
#define ECONNREFUSED 111   /* Connection refused */
#define EHOSTDOWN 112      /* Host is down */
#define EHOSTUNREACH 113   /* No route to host */
#define EALREADY 114       /* Operation already in progress */
#define EINPROGRESS 115    /* Operation now in progress */
#define ESTALE 116         /* Stale file handle */
#define EUCLEAN 117        /* Structure needs cleaning */
#define ENOTNAM 118        /* Not a XENIX named type file */
#define ENAVAIL 119        /* No XENIX semaphores available */
#define EISNAM 120         /* Is a named type file */
#define EREMOTEIO 121      /* Remote I/O error */
#define EDQUOT 122         /* Quota exceeded */

#define ENOMEDIUM 123    /* No medium found */
#define EMEDIUMTYPE 124  /* Wrong medium type */
#define ECANCELED 125    /* Operation Canceled */
#define ENOKEY 126       /* Required key not available */
#define EKEYEXPIRED 127  /* Key has expired */
#define EKEYREVOKED 128  /* Key has been revoked */
#define EKEYREJECTED 129 /* Key was rejected by service */

/* for robust mutexes */
#define EOWNERDEAD 130      /* Owner died */
#define ENOTRECOVERABLE 131 /* State not recoverable */

#define ERFKILL 132 /* Operation not possible due to RF-kill */

#define EHWPOISON 133 /* Memory page has hardware error */

// include/asm-generic/fcntl.h
#define F_DUPFD 0 /* dup */
#define F_GETFD 1 /* get close_on_exec */
#define F_SETFD 2 /* set/clear close_on_exec */
#define F_GETFL 3 /* get file->f_flags */
#define F_SETFL 4 /* set file->f_flags */
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7
#define F_SETOWN 8  /* for sockets. */
#define F_GETOWN 9  /* for sockets. */
#define F_SETSIG 10 /* for sockets. */

#define O_ACCMODE 00000003
#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#ifndef O_CREAT
#define O_CREAT 00000100 /* not fcntl */
#endif
#ifndef O_EXCL
#define O_EXCL 00000200 /* not fcntl */
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 00000400 /* not fcntl */
#endif
#ifndef O_TRUNC
#define O_TRUNC 00001000 /* not fcntl */
#endif
#ifndef O_APPEND
#define O_APPEND 00002000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 00004000
#endif
#ifndef O_DSYNC
#define O_DSYNC 00010000 /* used to be O_SYNC, see below */
#endif
#ifndef FASYNC
#define FASYNC 00020000 /* fcntl, for BSD compatibility */
#endif
#ifndef O_DIRECT
#define O_DIRECT 00040000 /* direct disk access hint */
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE 00100000
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY 00200000 /* must be a directory */
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 00400000 /* don't follow links */
#endif
#ifndef O_NOATIME
#define O_NOATIME 01000000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000 /* set close_on_exec */
#endif

/* Create a file descriptor with FD_CLOEXEC set. */
#define F_DUPFD_CLOEXEC (1024 + 6)

/* for F_[GET|SET]FL */
#define FD_CLOEXEC 1 /* actually anything with low bit set goes */

// include/linux/fb.h
#define FB_TYPE_PACKED_PIXELS 0      /* Packed Pixels	*/
#define FB_TYPE_PLANES 1             /* Non interleaved planes */
#define FB_TYPE_INTERLEAVED_PLANES 2 /* Interleaved planes	*/
#define FB_TYPE_TEXT 3               /* Text/attributes	*/
#define FB_TYPE_VGA_PLANES 4         /* EGA/VGA planes	*/
#define FB_TYPE_FOURCC 5             /* Type identified by a V4L2 FOURCC */

#define FB_VISUAL_MONO01 0             /* Monochr. 1=Black 0=White */
#define FB_VISUAL_MONO10 1             /* Monochr. 1=White 0=Black */
#define FB_VISUAL_TRUECOLOR 2          /* True color	*/
#define FB_VISUAL_PSEUDOCOLOR 3        /* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR 4        /* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR 5 /* Pseudo color readonly */
#define FB_VISUAL_FOURCC 6             /* Visual identified by a V4L2 FOURCC */

struct fb_fix_screeninfo {
  char          id[16];      /* identification string eg "TT Builtin" */
  unsigned long smem_start;  /* Start of frame buffer mem */
                             /* (physical address) */
  __u32         smem_len;    /* Length of frame buffer mem */
  __u32         type;        /* see FB_TYPE_*		*/
  __u32         type_aux;    /* Interleave for interleaved Planes */
  __u32         visual;      /* see FB_VISUAL_*		*/
  __u16         xpanstep;    /* zero if no hardware panning  */
  __u16         ypanstep;    /* zero if no hardware panning  */
  __u16         ywrapstep;   /* zero if no hardware ywrap    */
  __u32         line_length; /* length of a line in bytes    */
  unsigned long mmio_start;  /* Start of Memory Mapped I/O   */
                             /* (physical address) */
  __u32 mmio_len;            /* Length of Memory Mapped I/O  */
  __u32 accel;               /* Indicate to driver which	*/
                             /*  specific chip/card we have	*/
  __u16 capabilities;        /* see FB_CAP_*			*/
  __u16 reserved[2];         /* Reserved for future compatibility */
};

// assumed myself, pty
#define TIOCSPTLCK 0x40045431
#define TIOCGPTN 0xffffffff80045430

#define POLLIN 0x0001
#define POLLPRI 0x0002
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020
typedef unsigned int nfds_t;
struct pollfd {
  int   fd;      /* file descriptor */
  short events;  /* requested events */
  short revents; /* returned events */
};

typedef struct {
  uint16_t sa_family;
  char     sa_data[];
} sockaddr_linux;

#define SOCK_CLOEXEC 02000000
#define SOCK_NONBLOCK 04000

// include/linux/sched.h
/*
 * cloning flags:
 */
#define CSIGNAL 0x000000ff  /* signal mask to be sent at exit */
#define CLONE_VM 0x00000100 /* set if VM shared between processes */
#define CLONE_FS 0x00000200 /* set if fs info shared between processes */
#define CLONE_FILES                                                            \
  0x00000400 /* set if open files shared between processes                     \
              */
#define CLONE_SIGHAND                                                          \
  0x00000800 /* set if signal handlers and blocked signals shared */
#define CLONE_PIDFD 0x00001000 /* set if a pidfd should be placed in parent */
#define CLONE_PTRACE                                                           \
  0x00002000 /* set if we want to let tracing continue on the child too */
#define CLONE_VFORK                                                            \
  0x00004000 /* set if the parent wants the child to wake it up on mm_release  \
              */
#define CLONE_PARENT                                                           \
  0x00008000 /* set if we want to have the same parent as the cloner */
#define CLONE_THREAD 0x00010000         /* Same thread group? */
#define CLONE_NEWNS 0x00020000          /* New mount namespace group */
#define CLONE_SYSVSEM 0x00040000        /* share system V SEM_UNDO semantics */
#define CLONE_SETTLS 0x00080000         /* create a new TLS for the child */
#define CLONE_PARENT_SETTID 0x00100000  /* set the TID in the parent */
#define CLONE_CHILD_CLEARTID 0x00200000 /* clear the TID in the child */
#define CLONE_DETACHED 0x00400000       /* Unused, ignored */
#define CLONE_UNTRACED                                                         \
  0x00800000 /* set if the tracing process can't force CLONE_PTRACE on this    \
                clone */
#define CLONE_CHILD_SETTID 0x01000000 /* set the TID in the child */
#define CLONE_NEWCGROUP 0x02000000    /* New cgroup namespace */
#define CLONE_NEWUTS 0x04000000       /* New utsname namespace */
#define CLONE_NEWIPC 0x08000000       /* New ipc namespace */
#define CLONE_NEWUSER 0x10000000      /* New user namespace */
#define CLONE_NEWPID 0x20000000       /* New pid namespace */
#define CLONE_NEWNET 0x40000000       /* New network namespace */
#define CLONE_IO 0x80000000           /* Clone io context */

// include/linux/eventpoll.h
#define EPOLL_CLOEXEC O_CLOEXEC

/* Valid opcodes to issue to sys_epoll_ctl() */
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* Epoll event masks */
#define EPOLLIN (__poll_t)0x00000001
#define EPOLLPRI (__poll_t)0x00000002
#define EPOLLOUT (__poll_t)0x00000004
#define EPOLLERR (__poll_t)0x00000008
#define EPOLLHUP (__poll_t)0x00000010
#define EPOLLNVAL (__poll_t)0x00000020
#define EPOLLRDNORM (__poll_t)0x00000040
#define EPOLLRDBAND (__poll_t)0x00000080
#define EPOLLWRNORM (__poll_t)0x00000100
#define EPOLLWRBAND (__poll_t)0x00000200
#define EPOLLMSG (__poll_t)0x00000400
#define EPOLLRDHUP (__poll_t)0x00002000

/*
 * Internal flag - wakeup generated by io_uring, used to detect recursion back
 * into the io_uring poll handler.
 */
#define EPOLL_URING_WAKE ((__poll_t)(1U << 27))

/* Set exclusive wakeup mode for the target file descriptor */
#define EPOLLEXCLUSIVE ((__poll_t)(1U << 28))

/*
 * Request the handling of system wakeup events so as to prevent system suspends
 * from happening while those events are being processed.
 *
 * Assuming neither EPOLLET nor EPOLLONESHOT is set, system suspends will not be
 * re-allowed until epoll_wait is called again after consuming the wakeup
 * event(s).
 *
 * Requires CAP_BLOCK_SUSPEND
 */
#define EPOLLWAKEUP ((__poll_t)(1U << 29))

/* Set the One Shot behaviour for the target file descriptor */
#define EPOLLONESHOT ((__poll_t)(1U << 30))

/* Set the Edge Triggered behaviour for the target file descriptor */
#define EPOLLET ((__poll_t)(1U << 31))

/*
 * On x86-64 make the 64bit structure have the same alignment as the
 * 32bit structure. This makes 32bit emulation easier.
 *
 * UML/x86_64 needs the same packing as x86_64
 */
#define EPOLL_PACKED __attribute__((packed))
typedef unsigned int __poll_t;
struct epoll_event {
  __poll_t events;
  __u64    data;
} EPOLL_PACKED;

struct ucred {
  pid_t    pid; /* Process ID of the sending process */
  uid_t    uid; /* User ID of the sending process */
  unsigned gid; /* Group ID of the sending process */
};

struct msghdr_linux {
  void         *msg_name;    /* Socket name          */
  int           msg_namelen; /* Length of name       */
  struct iovec *msg_iov;     /* Data blocks          */
  size_t        msg_iovlen;  /* Number of blocks     */
  void  *msg_control; /* Per protocol magic (eg BSD file descriptor passing) */
  size_t msg_controllen; /* Length of cmsg list */
  unsigned int msg_flags;
};

// include/asm-generic/signal.h
#define __BITS_PER_LONG 64
#define _NSIG 64
#define _NSIG_BPW __BITS_PER_LONG
#define _NSIG_WORDS (_NSIG / _NSIG_BPW)

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPOLL SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR 30
#define SIGSYS 31
#define SIGUNUSED 31

/* These should not be considered constants from userland.  */
#define SIGRTMIN 32
#ifndef SIGRTMAX
#define SIGRTMAX _NSIG
#endif

#if !defined MINSIGSTKSZ || !defined SIGSTKSZ
#define MINSIGSTKSZ 2048
#define SIGSTKSZ 8192
#endif

// typedef struct {
//   unsigned long sig[_NSIG_WORDS];
// } sigset_t;

// !! Modified for ease of use! If _NSIG_WORDS > 1, this will NOT work!
typedef uint64_t sigset_t;

// include/asm-generic/signal-defs.h
#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP 0x00000001
#endif
#ifndef SA_NOCLDWAIT
#define SA_NOCLDWAIT 0x00000002
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO 0x00000004
#endif
/* 0x00000008 used on alpha, mips, parisc */
/* 0x00000010 used on alpha, parisc */
/* 0x00000020 used on alpha, parisc, sparc */
/* 0x00000040 used on alpha, parisc */
/* 0x00000080 used on parisc */
/* 0x00000100 used on sparc */
/* 0x00000200 used on sparc */
#define SA_UNSUPPORTED 0x00000400
#define SA_EXPOSE_TAGBITS 0x00000800
/* 0x00010000 used on mips */
/* 0x00800000 used for internal SA_IMMUTABLE */
/* 0x01000000 used on x86 */
/* 0x02000000 used on x86 */
/*
 * New architectures should not define the obsolete
 *	SA_RESTORER	0x04000000
 */
#ifndef SA_ONSTACK
#define SA_ONSTACK 0x08000000
#endif
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif
#ifndef SA_NODEFER
#define SA_NODEFER 0x40000000
#endif
#ifndef SA_RESETHAND
#define SA_RESETHAND 0x80000000
#endif
#define SA_RESTORER 0x04000000

#define SIG_BLOCK 0   /* for blocking signals */
#define SIG_UNBLOCK 1 /* for unblocking signals */
#define SIG_SETMASK 2 /* for setting the signal mask */

typedef void          __signalfn_t(int);
typedef __signalfn_t *__sighandler_t;

typedef void           __restorefn_t(void);
typedef __restorefn_t *__sigrestore_t;

#define SIG_DFL ((__sighandler_t)0)    /* default signal handling */
#define SIG_IGN ((__sighandler_t)1)    /* ignore signal */
#define SIG_ERR ((__sighandler_t) - 1) /* error return from signal */

// musl sources (include/asm/signal.h was bad)
struct sigaction {
  void (*sa_handler)(int);
  unsigned long sa_flags;
  void (*sa_restorer)(void);
  sigset_t sa_mask;
};

// include/asm/sigcontext.h & valgrind sources
struct fpstate {
  uint16_t cwd;
  uint16_t swd;
  uint16_t twd; /* Note this is not the same as the 32bit/x87/FSAVE twd */
  uint16_t fop;
  uint64_t rip;
  uint64_t rdp;
  uint32_t mxcsr;
  uint32_t mxcsr_mask;
  uint32_t st_space[32];  /* 8*16 bytes for each FP-reg */
  uint32_t xmm_space[64]; /* 16*16 bytes for each XMM-reg  */
  uint32_t reserved2[24];
};

struct sigcontext {
  unsigned long   r8;
  unsigned long   r9;
  unsigned long   r10;
  unsigned long   r11;
  unsigned long   r12;
  unsigned long   r13;
  unsigned long   r14;
  unsigned long   r15;
  unsigned long   rdi;
  unsigned long   rsi;
  unsigned long   rbp;
  unsigned long   rbx;
  unsigned long   rdx;
  unsigned long   rax;
  unsigned long   rcx;
  unsigned long   rsp;
  unsigned long   rip;
  unsigned long   eflags; /* RFLAGS */
  unsigned short  cs;
  unsigned short  gs;
  unsigned short  fs;
  unsigned short  ss; /* __pad0 */
  unsigned long   err;
  unsigned long   trapno;
  unsigned long   oldmask;
  unsigned long   cr2;
  struct fpstate *fpstate; /* zero when no FPU context */
  unsigned long   reserved1[8];
};

// include/linux/time.h
struct itimerval {
  struct timeval it_interval; /* timer interval */
  struct timeval it_value;    /* current value */
};

#define ITIMER_REAL 0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF 2

// include/linux/input.h
struct input_event {
  uint64_t sec;
  uint64_t usec;
  __u16    type;
  __u16    code;
  __s32    value;
};

// include/linux/linux-event-codes.h
#include "linux_event_codes.h"

#endif
