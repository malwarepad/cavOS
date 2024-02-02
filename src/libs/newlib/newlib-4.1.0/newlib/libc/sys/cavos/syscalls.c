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

void _exit(int code) { asm volatile("int $0x80" ::"a"(1), "b"(code)); }
int  close(int file) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(6), "b"(file));
  return ret;
}
char  *__env[1] = {0};
char **environ = __env;
int    execve(char *name, char **argv, char **env) {
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
int getpid(void) {
  uint32_t res;
  asm volatile("int $0x80" : "=a"(res) : "a"(20));
  return res;
}
int isatty(int file) { return file == 1 || file == 0; }
int kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}
int link(char *old, char *new) {
  errno = EMLINK;
  return -1;
}
int lseek(int file, int ptr, int dir) {
  int ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "a"(19), "b"(file), "c"(ptr), "d"(dir));
  return ret;
}
int open(const char *name, int flags, ...) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(5), "b"(name), "c"(flags), "d"(0));
  return ret;
}
int read(int file, char *ptr, int len) {
  uint32_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(3), "b"(file), "c"(ptr), "d"(len));
  return ret;
}
caddr_t sbrk(int incr) {
  uint32_t start_end;
  asm volatile("int $0x80" : "=a"(start_end) : "a"(403));
  asm volatile("int $0x80" ::"a"(404), "b"(start_end + incr));

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
  asm volatile("int $0x80" ::"a"(4), "b"(file), "c"(ptr), "d"(len));
  return len;
}
// int gettimeofday(struct timeval *p, struct timezone *z);