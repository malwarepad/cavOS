#include <linux.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

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
