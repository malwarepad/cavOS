#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#undef errno
extern int errno;

void _exit() {}
int  close(int file) { return -1; }
// char **environ; pointer to array of char * strings that define the current
// environment variables
int execve(char *name, char **argv, char **env) {
  errno = ENOMEM;
  return -1;
}
int fork() {
  errno = EAGAIN;
  return -1;
}
int fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}
int getpid(void) { return 1; }
int isatty(int file) { return 1; }
int kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}
int link(char *old, char *new) {
  errno = EMLINK;
  return -1;
}
int     lseek(int file, int ptr, int dir) { return 0; }
int     open(const char *name, int flags, ...) { return -1; }
int     read(int file, char *ptr, int len) { return 0; }
caddr_t sbrk(int incr) {
  uint32_t start_end;
  asm volatile("int $0x80" : "=a"(start_end) : "a"(6));
  asm volatile("int $0x80" ::"a"(7), "b"(start_end + incr));

  return (caddr_t)start_end;
}
int stat(const char *file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}
clock_t times(struct tms *buf) { return 0; }
int     unlink(char *name) {
  errno = ENOENT;
  return -1;
}
int wait(int *status) {
  errno = ECHILD;
  return -1;
}
int write(int file, char *ptr, int len) {
  asm volatile("int $0x80" ::"a"(8), "b"(ptr), "c"(len));
  return len;
}
// int gettimeofday(struct timeval *p, struct timezone *z);