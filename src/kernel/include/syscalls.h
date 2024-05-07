#include "isr.h"
#include "types.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

typedef struct iovec {
  void  *iov_base; /* Pointer to data.  */
  size_t iov_len;  /* Length of data.  */
} iovec;

typedef struct stat {
  uint32_t st_dev;     // Device ID
  uint32_t st_ino;     // inode number
  uint32_t st_mode;    // File mode
  uint32_t st_nlink;   // Number of hard links
  uint32_t st_uid;     // User ID of owner
  uint32_t st_gid;     // Group ID of owner
  uint32_t st_rdev;    // Device ID (if special file)
  uint32_t st_size;    // Total size, in bytes
  uint32_t st_blksize; // Optimal block size for I/O
  uint32_t st_blocks;  // Number of 512B blocks allocated
  uint64_t st_atime;   // Time of last access
  uint64_t st_mtime;   // Time of last modification
  uint64_t st_ctime;   // Time of last status change
} stat;

void registerSyscall(uint32_t id, void *handler);
void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();
void initiateSyscallInst();

#endif
