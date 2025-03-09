#include <linux.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#define SYSCALL_NANOSLEEP 35
static int syscallNanosleep(struct timespec *duration, struct timespec *rem) {
  dbgSysExtraf("sec{%ld} nsec{%ld}", duration->tv_sec, duration->tv_nsec);
  if (duration->tv_sec < 0)
    return -EINVAL;

  size_t ms = duration->tv_sec * 1000 + duration->tv_nsec / 1000000;
  currentTask->sleepUntil = timerTicks + ms;
  while (currentTask->sleepUntil > timerTicks) {
    // optimized on the scheduler but yk
    handControl();
  }

  currentTask->sleepUntil = 0; // reset to not strain the scheduler
  return 0;
}

#define SYSCALL_CLOCK_GETTIME 228
static size_t syscallClockGettime(int which, timespec *spec) {
  switch (which) {
  case CLOCK_MONOTONIC: // <- todo
  case CLOCK_REALTIME: {
    spec->tv_sec = timerBootUnix + timerTicks / 1000;
    size_t remainingInMs = timerTicks - (spec->tv_sec * 1000);
    spec->tv_nsec = remainingInMs * 1000000;
    // todo: accurancy
    return 0;
    break;
  }
  default:
    dbgSysStubf("clock not supported\n");
    return ERR(EINVAL);
    break;
  }
}

void syscallsRegClock() {
  registerSyscall(SYSCALL_NANOSLEEP, syscallNanosleep);
  registerSyscall(SYSCALL_CLOCK_GETTIME, syscallClockGettime);
}
