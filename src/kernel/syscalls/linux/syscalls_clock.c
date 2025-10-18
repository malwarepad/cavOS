#include <linux.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

#define SYSCALL_NANOSLEEP 35
static size_t syscallNanosleep(struct timespec *duration,
                               struct timespec *rem) {
  dbgSysExtraf("sec{%ld} nsec{%ld}", duration->tv_sec, duration->tv_nsec);
  if (duration->tv_sec < 0)
    return -EINVAL;

  size_t ms = duration->tv_sec * 1000 + duration->tv_nsec / 1000000;
  currentTask->forcefulWakeupTimeUnsafe = timerTicks + ms;
  currentTask->state = TASK_STATE_BLOCKED;
  do
    handControl();
  while (currentTask->forcefulWakeupTimeUnsafe > timerTicks);
  assert(!currentTask->forcefulWakeupTimeUnsafe);
  if (signalsPendingQuick(currentTask))
    return ERR(EINTR);
  return 0;
}

void ms_to_timeval(uint64_t ms, timeval *tv) {
  tv->tv_sec = ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
}

uint64_t timeval_to_ms(struct timeval tv) {
  return (uint64_t)tv.tv_sec * 1000 + DivRoundUp(tv.tv_usec, 1000);
}

#define SYSCALL_SETITIMER 38
static size_t syscallSetitimer(int which, struct itimerval *value,
                               struct itimerval *old) {
  if (which != 0) { // only ITIMER_REAL supported
    dbgSysFailf("todo other than ITIMER_REAL");
    return ERR(ENOSYS);
  }

  uint64_t rtAt = atomicRead64(&currentTask->infoSignals->itimerReal.at);
  uint64_t rtReset = atomicRead64(&currentTask->infoSignals->itimerReal.reset);

  if (old) {
    uint64_t realValue = MAX(0, rtAt - timerTicks);
    ms_to_timeval(realValue, &old->it_value);
    ms_to_timeval(rtReset, &old->it_interval);
  }

  if (value) {
    uint64_t targValue = timeval_to_ms(value->it_value);
    uint64_t targInterval = timeval_to_ms(value->it_interval);

    dbgSysExtraf("val{%ld} int{%ld}", targValue, targInterval);

    asm volatile("cli"); // just in case
    if (targValue)
      atomicWrite64(&currentTask->infoSignals->itimerReal.at,
                    timerTicks + targValue);
    else
      atomicWrite64(&currentTask->infoSignals->itimerReal.at, 0);
    atomicWrite64(&currentTask->infoSignals->itimerReal.reset, targInterval);
    asm volatile("sti");
  }

  return 0;
}

#define SYSCALL_CLOCK_GETTIME 228
static size_t syscallClockGettime(int which, timespec *spec) {
  switch (which) {
  case CLOCK_MONOTONIC: // <- todo
  case 6:               // CLOCK_MONOTONIC_COARSE
  case 4:               // CLOCK_MONOTONIC_RAW
  case CLOCK_REALTIME: {
    size_t time = timerBootUnix * 1000 + timerTicks;
    spec->tv_sec = time / 1000;
    spec->tv_nsec = (time % 1000) * 1000000;
    // todo: accurancy
    return 0;
    break;
  }
  case CLOCK_BOOTTIME: {
    size_t time = timerBootUnix * 1000;
    spec->tv_sec = time / 1000;
    spec->tv_nsec = (time % 1000) * 1000000;
    return 0;
    break;
  }
  default:
    dbgSysStubf("clock not supported\n");
    return ERR(EINVAL);
    break;
  }
}

#define SYSCALL_CLOCK_GETRES 229
static size_t syscallClockGetres(int which, timespec *spec) {
  spec->tv_nsec = 1000000; // 1ms for every clock
  return 0;
}

void syscallsRegClock() {
  registerSyscall(SYSCALL_NANOSLEEP, syscallNanosleep);
  registerSyscall(SYSCALL_CLOCK_GETTIME, syscallClockGettime);
  registerSyscall(SYSCALL_CLOCK_GETRES, syscallClockGetres);
  registerSyscall(SYSCALL_SETITIMER, syscallSetitimer);
}
