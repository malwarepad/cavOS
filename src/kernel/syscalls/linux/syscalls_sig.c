#include <linux.h>
#include <syscalls.h>

/*#define SYSCALL_RT_SIGACTION 13
static size_t syscallRtSigaction(int sig, const struct sigaction *act,
                                 struct sigaction *oact, size_t sigsetsize) {
  return ERR(ENOSYS);
}

#define SYSCALL_RT_SIGPROCMASK 14
static size_t syscallRtSigprocmask(int how, __sigset_t *nset, __sigset_t *oset,
                                   size_t sigsetsize) {
  return ERR(ENOSYS);
}*/

void syscallRegSig() {
  // a
  // registerSyscall(SYSCALL_RT_SIGACTION, syscallRtSigaction);
  // registerSyscall(SYSCALL_RT_SIGPROCMASK, syscallRtSigprocmask);
}