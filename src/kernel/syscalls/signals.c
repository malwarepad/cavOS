#include <bootloader.h>
#include <gdt.h>
#include <paging.h>
#include <stdatomic.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>
#include <util.h>

// Signal dispatcher, generator & general utility
// Copyright (C) 2025 Panagiotis

#define currentTask (youWillNotUseThisInNoWayImaginable())

SignalInternal signalInternalDecisions[_NSIG] = {0};

// stack is laid out like this afterwards
// * uint64_t retaddr;
// * struct sigcontext ucontext;
// * struct siginfo info; (optional)
// * struct fpstate fp;

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

void signalsAsmPassedToUcontext(const AsmPassedInterrupt *passed,
                                struct sigcontext        *ucontext) {
  ucontext->r8 = passed->r8;
  ucontext->r9 = passed->r9;
  ucontext->r10 = passed->r10;
  ucontext->r11 = passed->r11;
  ucontext->r12 = passed->r12;
  ucontext->r13 = passed->r13;
  ucontext->r14 = passed->r14;
  ucontext->r15 = passed->r15;
  ucontext->rdi = passed->rdi;
  ucontext->rsi = passed->rsi;
  ucontext->rbp = passed->rbp;
  ucontext->rbx = passed->rbx;
  ucontext->rdx = passed->rdx;
  ucontext->rax = passed->rax;
  ucontext->rcx = passed->rcx;
  ucontext->rsp = passed->usermode_rsp;
  ucontext->rip = passed->rip;
  ucontext->eflags = passed->rflags;
  ucontext->cs = passed->cs;
  // ucontext->gs = gs;
  // ucontext->fs = fs;
  ucontext->ss = passed->usermode_ss;
  ucontext->err = 0;
  ucontext->trapno = 0;
  ucontext->oldmask = 0; // to be filled!
  ucontext->cr2 = 0;
  ucontext->fpstate = 0; // to be filled!
  memset(ucontext->reserved1, 0, sizeof(ucontext->reserved1));
}

void signalsUcontextToAsmPassed(const struct sigcontext *ucontext,
                                AsmPassedInterrupt      *passed) {
  passed->r8 = ucontext->r8;
  passed->r9 = ucontext->r9;
  passed->r10 = ucontext->r10;
  passed->r11 = ucontext->r11;
  passed->r12 = ucontext->r12;
  passed->r13 = ucontext->r13;
  passed->r14 = ucontext->r14;
  passed->r15 = ucontext->r15;
  passed->rdi = ucontext->rdi;
  passed->rsi = ucontext->rsi;
  passed->rbp = ucontext->rbp;
  passed->rbx = ucontext->rbx;
  passed->rdx = ucontext->rdx;
  passed->rax = ucontext->rax;
  passed->rcx = ucontext->rcx;
  passed->usermode_rsp = ucontext->rsp;
  passed->rip = ucontext->rip;
  passed->rflags = ucontext->eflags;
  passed->cs = ucontext->cs;
  // *gs = ucontext->gs;
  // *fs = ucontext->fs;
  passed->usermode_ss = ucontext->ss;
  passed->error = ucontext->err;
  passed->interrupt = ucontext->trapno;
  // passed->oldmask = ucontext->oldmask;
  // passed->cr2 = ucontext->cr2;

  // struct fpstate *fpstate;
  // unsigned long   reserved1[8];
}

// fast function to tell if a signal is pending (for constant-pull syscalls)
bool signalsPendingQuick(void *taskPtr) {
  Task    *task = (Task *)taskPtr;
  sigset_t pendingList = atomicBitmapGet(&task->sigPendingList);
  return pendingList & ~task->sigBlockList;
}

// these functions should be fairly bare-bones as they are invoked from
// interrupt contexts and unsafe syscall positions!
int signalsPendingDecide(Task *task) {
  // find a single valid pending signal
  int signal = -1;
  while (true) {
    uint64_t pendingList = atomicBitmapGet(&task->sigPendingList);
    signal = signalsPendingFind(pendingList);
    if (signal == -1) // fail
      return signal;
    if (task->sigBlockList & (1 << signal)) // blocked but somehow hit
      atomicBitmapClear(&task->sigPendingList, signal);
    else // valid signal
      return signal;
  }
}

// ! alignemnt for below functions: the structs NEED TO BE CALCULATED so that
// ! rsp is ALWAYS 16 byte aligned at the end of the day (test in vmware)

// this runs after syscall.. stuff from Task* are cleared so they need to be
// passed (rsp as pointer because it will be modified and return properly)
void signalsPendingHandleSys(void *taskPtr, uint64_t *rsp,
                             AsmPassedInterrupt *registers) {
  Task *task = (Task *)taskPtr;

  int signal = signalsPendingDecide(task);
  if (signal == -1) // continue as per usual
    return;

  signalHitf("--- %ld [signals] %s hit (%d:sys) ---\n", task->id,
             signalStr(signal), signal);

  struct sigaction *action = &task->infoSignals->signals[signal];
  __sighandler_t    handler =
      (__sighandler_t)atomicRead64((size_t *)(size_t)&action->sa_handler);
  if (handler == SIG_DFL) {
    switch (signalInternalDecisions[signal]) {
    case SIGNAL_INTERNAL_CORE:
    case SIGNAL_INTERNAL_TERM:
      signalHitf("--- %ld [signals] Killing! ---\n", task->id);
      taskKill(task->id, 128 + signal);
      break;
    case SIGNAL_INTERNAL_IGN:
      handler = SIG_IGN; // hence no else if
      break;
    case SIGNAL_INTERNAL_STOP:
    case SIGNAL_INTERNAL_CONT:
    default:
      debugf("[signals] Todo! stop & default: decision{%d}\n",
             signalInternalDecisions[signal]);
      panic();
      break;
    }
  }

  if (handler == SIG_IGN) {
    signalHitf("--- %ld [signals] ignored! ---\n", task->id);
    atomicBitmapClear(&task->sigPendingList, signal);
    return;
  }

  // (also SA_NODEFER)
  sigset_t oldMask = task->sigBlockList;
  task->sigBlockList &= ~((1 << signal) | atomicRead64(&action->sa_mask));
  atomicBitmapClear(&task->sigPendingList, signal); // now that it is blocked

  // establish a "safe" struct, that we could theoretically safely iretq to
  AsmPassedInterrupt oldstate = {0};
  memcpy(&oldstate, registers, sizeof(AsmPassedInterrupt));
  // comes from syscall, fix it up (from task.c)
  oldstate.cs = GDT_USER_CODE | DPL_USER;
  oldstate.ds = GDT_USER_DATA | DPL_USER;
  oldstate.rflags = oldstate.r11;
  oldstate.rip = oldstate.rcx; // extra
  oldstate.usermode_rsp = *rsp;
  oldstate.usermode_ss = GDT_USER_DATA | DPL_USER;
  // since the scheduler, our fpu state might've changed (from task.c)
  asm volatile(" fxsave %0 " ::"m"(task->fpuenv));
  asm("stmxcsr (%%rax)" : : "a"(&task->mxcsr));

  size_t sigrsp = *rsp;

  // for alignment. have a (relatively) stable base
  sigrsp = (sigrsp / PAGE_SIZE) * PAGE_SIZE;

  // get down to avoid the red zone
  sigrsp -= 128;

  sigrsp -= sizeof(struct fpstate);
  struct fpstate *fpu = (struct fpstate *)sigrsp;
  memcpy(fpu, task->fpuenv, sizeof(struct fpstate));

  sigrsp -= sizeof(struct sigcontext);
  struct sigcontext *ucontext = (struct sigcontext *)sigrsp;
  signalsAsmPassedToUcontext(&oldstate, ucontext);
  ucontext->oldmask = oldMask;
  ucontext->fpstate = fpu;

  sigrsp -= sizeof(void *);
  *((void **)sigrsp) =
      (void *)atomicRead64((size_t *)(size_t)&action->sa_restorer);

  // point the main rsp, rsp, etc
  // registers->usermode_rsp = sigrsp; // to retaddr
  *rsp = sigrsp; // to retaddr
  // registers->rip = (size_t)action->sa_handler;
  registers->rcx = (size_t)handler;
  registers->rdi = signal;
}
void signalsPendingHandleSched(void *taskPtr) {
  Task *task = (Task *)taskPtr;

  int signal = signalsPendingDecide(task);
  if (signal == -1) // continue as per usual
    return;

  signalHitf("--- %ld [signals] %s hit (%d:sched) ---\n", task->id,
             signalStr(signal), signal);

  struct sigaction *action = &task->infoSignals->signals[signal];
  __sighandler_t    handler =
      (__sighandler_t)atomicRead64((size_t *)(size_t)&action->sa_handler);
  if (handler == SIG_DFL) {
    switch (signalInternalDecisions[signal]) {
    case SIGNAL_INTERNAL_CORE:
    case SIGNAL_INTERNAL_TERM:
      signalHitf("--- %ld [signals] Killing! ---\n", task->id);
      task->tmpRecV = signal;
      task->state = TASK_STATE_SIGKILLED;
      handler = SIG_IGN;
      debugf("[signals] Todo: actually kill from scheduler context!\n");
      break;
    case SIGNAL_INTERNAL_IGN:
      handler = SIG_IGN; // hence no else if
      break;
    case SIGNAL_INTERNAL_STOP:
    case SIGNAL_INTERNAL_CONT:
    default:
      debugf("[signals] Todo! stop & default: decision{%d}\n",
             signalInternalDecisions[signal]);
      panic();
      break;
    }
  }

  if (handler == SIG_IGN) {
    signalHitf("--- %ld [signals] ignored! ---\n", task->id);
    atomicBitmapClear(&task->sigPendingList, signal);
    return;
  }

  // (also SA_NODEFER)
  sigset_t oldMask = task->sigBlockList;
  task->sigBlockList &= ~((1 << signal) | atomicRead64(&action->sa_mask));
  atomicBitmapClear(&task->sigPendingList, signal); // now that it is blocked

  AsmPassedInterrupt oldstate = {0};
  memcpy(&oldstate, &task->registers, sizeof(AsmPassedInterrupt));

  // get down to avoid the red zone (will be left-overs but make sure)
  task->registers.usermode_rsp -= 128;

  // get down another page so we can go to the end and put our stuff
  task->registers.usermode_rsp -= PAGE_SIZE;
  task->registers.usermode_rsp =
      (task->registers.usermode_rsp / PAGE_SIZE) * PAGE_SIZE; // align properly

  // ensure we haven't ran out of stack space
  size_t regionPhys =
      VirtualToPhysicalL(task->infoPd->pagedir, task->registers.usermode_rsp);
  assert(regionPhys);

  // make a region which we access by it's end (max 4KiB // PAGE_SIZE)
  int    top = PAGE_SIZE;
  size_t region = bootloader.hhdmOffset + regionPhys;

  top -= sizeof(struct fpstate);
  struct fpstate *fpu = (struct fpstate *)(region + top);
  memcpy(fpu, task->fpuenv, sizeof(struct fpstate));
  int fpuoffset = top;

  top -= sizeof(struct sigcontext);
  struct sigcontext *ucontext = (struct sigcontext *)(region + top);
  signalsAsmPassedToUcontext(&oldstate, ucontext);
  ucontext->oldmask = oldMask;

  // point fpstate properly
  ucontext->fpstate = (void *)(task->registers.usermode_rsp + fpuoffset);

  top -= sizeof(void *);
  *((void **)(region + top)) =
      (void *)atomicRead64((size_t *)(size_t)&action->sa_restorer);

  // point the main rsp, rsp, etc
  task->registers.usermode_rsp += top; // to retaddr
  task->registers.rip = (size_t)handler;
  task->registers.rdi = signal;
}

// this will be called strictly from a syscall handler environment
size_t signalsSigreturnSyscall(void *taskPtr) {
  Task *task = (Task *)taskPtr; // avoid currentTask

#if DEBUG_SYSCALLS_STRACE
  debugf("\n");
#endif

  uint64_t saveStackTop = task->syscallRsp;
  uint64_t saveStackBrowse = saveStackTop;

  // return ptr has already been pushed
  struct sigcontext *ucontext = (struct sigcontext *)saveStackBrowse;

  AsmPassedInterrupt regs = {0};
  signalsUcontextToAsmPassed(ucontext, &regs);

  task->sigBlockList = ucontext->oldmask & ~(SIGKILL | SIGSTOP);

  asm volatile("cli"); // we're using whileTssRsp which is strictly for sched
  AsmPassedInterrupt *iretqRsp =
      (AsmPassedInterrupt *)(task->whileTssRsp - sizeof(AsmPassedInterrupt));
  memcpy(iretqRsp, &regs, sizeof(AsmPassedInterrupt));

  memcpy(task->fpuenv, ucontext->fpstate, sizeof(task->fpuenv));
  task->mxcsr = ucontext->fpstate->mxcsr;
  asm volatile(" fxrstor %0 " ::"m"(task->fpuenv));

  task->systemCallInProgress = false;
  task->syscallRegs = 0;
  task->syscallRsp = 0;

  asm_finalize((size_t)iretqRsp,
               VirtualToPhysical((size_t)task->infoPd->pagedir));

  // will never be reached
  panic();
  return 0;
}
