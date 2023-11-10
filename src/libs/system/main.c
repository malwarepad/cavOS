#include <stdint.h>

#include "system.h"

// asm volatile("int $0x80" : "=a"(ret) : "a"(0), "b"(str));
// -O2 is the devil for those istg

void syscallTest(char *msg) {
  asm volatile("int $0x80" ::"a"(SYSCALL_TEST), "b"(msg));
}

void syscallExitTask(int return_code) {
  asm volatile("int $0x80" ::"a"(SYSCALL_EXIT_TASK), "b"(return_code));
}

uint32_t syscallGetPid() {
  uint32_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYSCALL_GETPID));
  return ret;
}

int syscallGetArgc() {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYSCALL_GETARGC));
  return ret;
}

char *syscallGetArgv(int curr) {
  char *ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYSCALL_GETARGV), "b"(curr));
  return ret;
}
