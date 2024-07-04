#include <linux.h>
#include <syscalls.h>

#define SYSCALL_RT_SIGACTION 13
static int syscallRtSigaction(int sig, const struct sigaction *act,
                              struct sigaction *oact, size_t sigsetsize) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::sigaction] sig{%d} act{%lx} oact{%lx} sigsetsize{%lx}\n",
         sig, act, oact, sigsetsize);
#endif

#if DEBUG_SYSCALLS_STUB
  debugf("[syscalls::sigaction] UNIMPLEMENTED!\n");
#endif
  return -ENOSYS;
}

#define SYSCALL_RT_SIGPROCMASK 14
static int syscallRtSigprocmask(int how, __sigset_t *nset, __sigset_t *oset,
                                size_t sigsetsize) {
#if DEBUG_SYSCALLS_ARGS
  debugf(
      "[syscalls::sigprocmask] how{%d} nset{%lx} oset{%lx} sigsetsize{%lx}\n",
      how, nset, oset, sigsetsize);
#endif

#if DEBUG_SYSCALLS_STUB
  debugf("[syscalls::sigprocmask] UNIMPLEMENTED!\n");
#endif
  return -ENOSYS;
}

void syscallRegSig() {
  registerSyscall(SYSCALL_RT_SIGACTION, syscallRtSigaction);
  registerSyscall(SYSCALL_RT_SIGPROCMASK, syscallRtSigprocmask);
}