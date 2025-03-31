#include <linux.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>

/*if (act && act->sa_handler != SIG_DFL && act->sa_handler != SIG_IGN &&
      act->sa_handler != SIG_ERR)
    debugf("%d %lx\n", sig, (size_t)act->sa_restorer);*/

size_t sigactionSupportedFlags = SA_RESTORER | SA_RESTART;

#define SYSCALL_RT_SIGACTION 13
static size_t syscallRtSigaction(int sig, const struct sigaction *act,
                                 struct sigaction *oact, size_t sigsetsize) {
  if (sig < 0 || sig > _NSIG)
    return ERR(EINVAL);
  if (sigsetsize < sizeof(sigset_t)) {
    dbgSysFailf("weird sigset size");
    return ERR(EINVAL);
  }
  if (sig >= SIGRTMIN) { // [min, +inf)
    dbgSysStubf("todo real-time signals");
    return ERR(ENOSYS);
  }

  if (oact) {
    TaskInfoSignal *info = currentTask->infoSignals;
    spinlockAcquire(&info->LOCK_SIGNAL);
    memcpy(oact, &info->signals[sig], sizeof(struct sigaction));
    spinlockRelease(&info->LOCK_SIGNAL);
  }
  if (act) {
    if (sig == SIGKILL || sig == SIGSTOP)
      return ERR(EINVAL);

    if (act->sa_flags & ~(sigactionSupportedFlags)) {
      dbgSigStubf("[signals::sigaction] Unsupported flags! flags{%lx}\n",
                  act->sa_flags);
      return ERR(ENOSYS);
    }

    if (!(act->sa_flags & SA_RESTORER)) {
      dbgSigStubf("[signals::sigaction] No restorer provided by libc! todo!\n");
      return ERR(ENOSYS);
    }

    TaskInfoSignal *info = currentTask->infoSignals;
    spinlockAcquire(&info->LOCK_SIGNAL);
    // do it manually to be safe with the masks
    struct sigaction *target = &info->signals[sig];
    target->sa_handler = act->sa_handler;
    target->sa_flags = act->sa_flags;
    target->sa_restorer = act->sa_restorer;
    target->sa_mask = act->sa_mask & ~(SIGKILL | SIGSTOP);
    spinlockRelease(&info->LOCK_SIGNAL);
  }

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

#define SYSCALL_RT_SIGRETURN 15
static size_t syscallRtSigreturn() {
  return signalsSigreturnSyscall(currentTask);
}

#define SYSCALL_KILL 62
static size_t syscallKill(int pid, int sig) {
  if (sig < 1 || sig > _NSIG)
    return ERR(EINVAL);

  if (sig >= SIGRTMIN) {
    dbgSigStubf("[signals::kill] Todo real-time signals!\n");
    return ERR(ENOSYS);
  }

  if (pid > 0) {
    // specific tgid
    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *target = firstTask;
    while (target) {
      if (target->tgid == pid)
        break;
      target = target->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);
    if (!target || target->state == TASK_STATE_DEAD)
      return ERR(ESRCH);
    atomicBitmapSet(&target->sigPendingList, sig);
  } else if (!pid) {
    // sent to every process in our group
    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *target = firstTask;
    while (target) {
      if (target->pgid == currentTask->pgid)
        atomicBitmapSet(&target->sigPendingList, sig);
      target = target->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);
  } else if (pid == -1) {
    // every single one (todo init // id 1)
    int cnt = 0;
    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *target = firstTask;
    while (target) {
      cnt++;
      atomicBitmapSet(&target->sigPendingList, sig);
      target = target->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);
    if (!cnt)
      return ERR(ESRCH);
  } else if (pid < -1) {
    // -pid process group
    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *target = firstTask;
    while (target) {
      if (target->pgid == -pid)
        atomicBitmapSet(&target->sigPendingList, sig);
      target = target->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);
  }

  return 0;
}

void syscallRegSig() {
  // a
  registerSyscall(SYSCALL_RT_SIGACTION, syscallRtSigaction);
  registerSyscall(SYSCALL_RT_SIGPROCMASK, syscallRtSigprocmask);
  registerSyscall(SYSCALL_RT_SIGRETURN, syscallRtSigreturn);
  registerSyscall(SYSCALL_KILL, syscallKill);
}