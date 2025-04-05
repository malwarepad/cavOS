#include <util.h>

// Utilities used inside source code
// Copyright (C) 2024 Panagiotis

const char *ANSI_RESET = "\e[0m";
const char *ANSI_BLACK = "\e[0;30m";
const char *ANSI_RED = "\e[0;31m";
const char *ANSI_GREEN = "\e[0;32m";
const char *ANSI_YELLOW = "\e[0;33m";
const char *ANSI_BLUE = "\e[0;34m";
const char *ANSI_PURPLE = "\e[0;35m";
const char *ANSI_CYAN = "\e[0;36m";
const char *ANSI_WHITE = "\e[0;37m";

const char *LINUX_ERRNO[37] = {
    "EPERM",  "ENOENT",  "ESRCH",   "EINTR",  "EIO",     "ENXIO",
    "E2BIG",  "ENOEXEC", "EBADF",   "ECHILD", "EAGAIN",  "ENOMEM",
    "EACCES", "EFAULT",  "ENOTBLK", "EBUSY",  "EEXIST",  "EXDEV",
    "ENODEV", "ENOTDIR", "EISDIR",  "EINVAL", "ENFILE",  "EMFILE",
    "ENOTTY", "ETXTBSY", "EFBIG",   "ENOSPC", "ESPIPE",  "EROFS",
    "EMLINK", "EPIPE",   "EDOM",    "ERANGE", "EDEADLK", "ENAMETOOLONG",
    "ENOLCK"};

const char *SIGNALS[34] = {
    "ZERO?",   "SIGHUP",  "SIGINT",    "SIGQUIT", "SIGILL",    "SIGTRAP",
    "SIGABRT", "SIGBUS",  "SIGFPE",    "SIGKILL", "SIGUSR1",   "SIGSEGV",
    "SIGUSR2", "SIGPIPE", "SIGALRM",   "SIGTERM", "SIGSTKFLT", "SIGCHLD",
    "SIGCONT", "SIGSTOP", "SIGTSTP",   "SIGTTIN", "SIGTTOU",   "SIGURG",
    "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH",  "SIGIO",
    "SIGPWR",  "SIGSYS",  "SIGUNUSED",
};
const char *sigStrDefault = "SIGRTn";
const char *signalStr(int signum) {
  if (signum < 32)
    return SIGNALS[signum];
  return sigStrDefault;
}

void memset(void *_dst, int val, size_t len) {
  asm volatile("pushf; cld; rep stosb; popf"
               :
               : "D"(_dst), "a"(val), "c"(len)
               : "memory");
}

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size) {
  asm volatile("pushf; cld; rep movsb; popf"
               :
               : "S"(srcptr), "D"(dstptr), "c"(size)
               : "memory");
  return dstptr;
}

void *memmove(void *dstptr, const void *srcptr, size_t size) {
  unsigned char       *dst = (unsigned char *)dstptr;
  const unsigned char *src = (const unsigned char *)srcptr;
  if (dst < src) {
    for (size_t i = 0; i < size; i++)
      dst[i] = src[i];
  } else {
    for (size_t i = size; i != 0; i--)
      dst[i - 1] = src[i - 1];
  }
  return dstptr;
}

int memcmp(const void *aptr, const void *bptr, size_t size) {
  const unsigned char *a = (const unsigned char *)aptr;
  const unsigned char *b = (const unsigned char *)bptr;
  for (size_t i = 0; i < size; i++) {
    if (a[i] < b[i])
      return -1;
    else if (b[i] < a[i])
      return 1;
  }
  return 0;
}

void atomicBitmapSet(volatile uint64_t *bitmap, unsigned int bit) {
  atomic_fetch_or((volatile _Atomic uint64_t *)bitmap, (1UL << bit));
}

void atomicBitmapClear(volatile uint64_t *bitmap, unsigned int bit) {
  atomic_fetch_and((volatile _Atomic uint64_t *)bitmap, ~(1UL << bit));
}

uint64_t atomicBitmapGet(volatile uint64_t *bitmap) {
  return atomic_load((volatile _Atomic uint64_t *)bitmap);
}

uint8_t atomicRead8(volatile uint8_t *target) {
  return atomic_load((volatile _Atomic uint8_t *)target);
}

uint16_t atomicRead16(volatile uint16_t *target) {
  return atomic_load((volatile _Atomic uint16_t *)target);
}

uint32_t atomicRead32(volatile uint32_t *target) {
  return atomic_load((volatile _Atomic uint32_t *)target);
}

uint64_t atomicRead64(volatile uint64_t *target) {
  return atomic_load((volatile _Atomic uint64_t *)target);
}

void atomicWrite8(volatile uint8_t *target, uint8_t value) {
  atomic_store((volatile _Atomic uint8_t *)target, value);
}

void atomicWrite16(volatile uint16_t *target, uint16_t value) {
  atomic_store((volatile _Atomic uint16_t *)target, value);
}

void atomicWrite32(volatile uint32_t *target, uint32_t value) {
  atomic_store((volatile _Atomic uint32_t *)target, value);
}

void atomicWrite64(volatile uint64_t *target, uint64_t value) {
  atomic_store((volatile _Atomic uint64_t *)target, value);
}

static unsigned long int next = 1;

int rand(void) {
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) { next = seed; }

// https://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
// cool function
void hexDump(const char *desc, const void *addr, const int len, int perLine,
             int (*f)(const char *fmt, ...)) {
  int                  i;
  unsigned char        buff[perLine + 1];
  const unsigned char *pc = (const unsigned char *)addr;

  if (desc != NULL)
    f("%s:\n", desc);

  if (len == 0) {
    f("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    f("  NEGATIVE LENGTH: %d\n", len);
    return;
  }

  for (i = 0; i < len; i++) {
    if ((i % perLine) == 0) {
      if (i != 0)
        f("  %s\n", buff);

      f("  %04x ", i);
    }

    f(" %02x", pc[i]);

    if ((pc[i] < 0x20) || (pc[i] > 0x7e))
      buff[i % perLine] = '.';
    else
      buff[i % perLine] = pc[i];
    buff[(i % perLine) + 1] = '\0';
  }

  while ((i % perLine) != 0) {
    f("   ");
    i++;
  }

  f("  %s\n", buff);
}
