#include <linux.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

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

// #define SYSCALL_NANOSLEEP 35
// static int syscallNanosleep() {
//   sleep(1);
//   return -1;
// }

#define SYSCALL_CLOCK_GETTIME 228
static int syscallClockGettime(int which, timespec *spec) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::gettime] which{%d} timespec{%lx}!\n", which, spec);
#endif

  switch (which) {
  case CLOCK_REALTIME:
    spec->tv_sec = timerTicks / 1000;
    size_t remainingInMs = timerTicks - (spec->tv_sec * 1000);
    spec->tv_nsec = remainingInMs * 1000000;
    // todo: accurancy
    return 0;
    break;
  default:
#if DEBUG_SYSCALLS_STUB
    debugf("[syscalls::gettime] UNIMPLEMENTED! which{%d} timespec{%lx}!\n",
           which, spec);
#endif
    return -1;
    break;
  }
}

void syscallsRegClock() {
  // registerSyscall(SYSCALL_NANOSLEEP, syscallNanosleep);
  registerSyscall(SYSCALL_CLOCK_GETTIME, syscallClockGettime);
}
