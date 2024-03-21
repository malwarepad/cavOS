#include "isr.h"
#include "types.h"

#ifndef SYSCALLS_H
#define SYSCALLS_H

typedef struct iovec {
  void  *iov_base; /* Pointer to data.  */
  size_t iov_len;  /* Length of data.  */
} iovec;

void registerSyscall(uint32_t id, void *handler);
void syscallHandler(AsmPassedInterrupt *regs);
void initiateSyscalls();

#endif
