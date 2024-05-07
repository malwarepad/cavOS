#include "fs_controller.h"
#include "isr.h"
#include "types.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

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

void registerSyscall(uint32_t id, void *handler);
void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();
void initiateSyscallInst();

int readHandler(OpenFile *fd, uint8_t *in, size_t limit);
int writeHandler(OpenFile *fd, uint8_t *out, size_t limit);
int ioctlHandler(OpenFile *fd, uint64_t request, void *arg);

#endif
