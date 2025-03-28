#include <stdatomic.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>

// Signal helper & utility
// Copyright (C) 2025 Panagiotis

SignalInternal signalInternalDecisions[_NSIG] = {0};

void initiateSignalDefs() {
  signalInternalDecisions[SIGABRT] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGALRM] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGBUS] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGCHLD] = SIGNAL_INTERNAL_IGN;
  // signalInternalDecisions[SIGCLD] = SIGNAL_INTERNAL_IGN;
  signalInternalDecisions[SIGCONT] = SIGNAL_INTERNAL_CONT;
  // signalInternalDecisions[SIGEMT] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGFPE] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGHUP] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGILL] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGINT] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGIO] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGIOT] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGKILL] = SIGNAL_INTERNAL_TERM;
  // signalInternalDecisions[SIGLOST] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGPIPE] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGPOLL] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGPROF] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGPWR] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGQUIT] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGSEGV] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGSTKFLT] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGSTOP] = SIGNAL_INTERNAL_STOP;
  signalInternalDecisions[SIGTSTP] = SIGNAL_INTERNAL_STOP;
  signalInternalDecisions[SIGSYS] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGTERM] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGTRAP] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGTTIN] = SIGNAL_INTERNAL_STOP;
  signalInternalDecisions[SIGTTOU] = SIGNAL_INTERNAL_STOP;
  signalInternalDecisions[SIGUNUSED] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGURG] = SIGNAL_INTERNAL_IGN;
  signalInternalDecisions[SIGUSR1] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGUSR2] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGVTALRM] = SIGNAL_INTERNAL_TERM;
  signalInternalDecisions[SIGXCPU] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGXFSZ] = SIGNAL_INTERNAL_CORE;
  signalInternalDecisions[SIGWINCH] = SIGNAL_INTERNAL_IGN;
}

int signalsPendingFind(uint64_t bitmap) {
  for (int i = 0; i < _NSIG; i++) {
    if (bitmap & (1 << i))
      return i;
  }
  return -1;
}

// this function should be fairly bare-bones as it is invoked from interrupt
// contexts and unsafe syscall positions!
void signalsPendingHandle(void *taskPtr) {
  Task *task = (Task *)taskPtr;

  // find a single valid pending signal
  int signal = -1;
  while (true) {
    uint64_t pendingList = atomicBitmapGet(&task->sigPendingList);
    signal = signalsPendingFind(pendingList);
    if (signal == -1)
      return;
    if (currentTask->sigBlockList & (1 << signal)) // blocked but somehow hit
      atomicBitmapClear(&currentTask->sigPendingList, signal);
    else // valid signal
      break;
  }

  // ensure it holds a pending signal now
  assert(signal != -1);

  panic();
}
