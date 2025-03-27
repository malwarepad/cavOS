#include <linux.h>
#include <syscalls.h>
#include <task.h>

#define SYSCALL_RT_SIGACTION 13
static size_t syscallRtSigaction(int sig, const struct sigaction *act,
                                 struct sigaction *oact, size_t sigsetsize) {
  dbgSysStubf("todo signals");
  return 0;
}

// I'm not afraid of any thread-safe stuff since we will only hit signal
// handlers after this syscall is completed and only we (meaning this thread)
// can change the mask. Reads will be performed afterwards anyways.
#define SYSCALL_RT_SIGPROCMASK 14
static size_t syscallRtSigprocmask(int how, sigset_t *nset, sigset_t *oset,
                                   size_t sigsetsize) {
  if (oset)
    *oset = currentTask->sigBlockList;
  if (nset) {
    uint64_t safe = *nset;
    safe &= ~(SIGKILL | SIGSTOP); // nuh uh!
    switch (how) {
    case SIG_BLOCK:
      currentTask->sigBlockList |= safe;
      break;
    case SIG_UNBLOCK:
      currentTask->sigBlockList &= ~(safe);
      break;
    case SIG_SETMASK:
      currentTask->sigBlockList = safe;
      break;
    default:
      return ERR(EINVAL);
      break;
    }
  }

  return 0;
}

void syscallRegSig() {
  // a
  registerSyscall(SYSCALL_RT_SIGACTION, syscallRtSigaction);
  registerSyscall(SYSCALL_RT_SIGPROCMASK, syscallRtSigprocmask);
}