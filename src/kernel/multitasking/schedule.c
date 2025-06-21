#include <bootloader.h>
#include <gdt.h>
#include <isr.h>
#include <malloc.h>
#include <paging.h>
#include <schedule.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>
#include <vmm.h>

// Clock-tick triggered scheduler
// Copyright (C) 2024 Panagiotis

#define SCHEDULE_DEBUG 0

extern TSSPtr *tssPtr;

void schedule(uint64_t rsp) {
  if (!tasksInitiated)
    return;

  // try to find a next task
  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;
  Task               *next = currentTask->next;
  if (!next)
    next = firstTask;

  int fullRun = 0;
  while (next->state != TASK_STATE_READY) {
    if (signalsRevivableState(next->state) && signalsPendingQuick(next)) {
      // back to the syscall handler which returns -EINTR (& handles the signal)
      assert(next->registers.cs & GDT_KERNEL_CODE);
      next->forcefulWakeupTimeUnsafe = 0; // needed
      next->state = TASK_STATE_READY;
      break;
    }
    if (next->forcefulWakeupTimeUnsafe &&
        next->forcefulWakeupTimeUnsafe <= timerTicks) {
      // no race! the task has to already have been suspended to end up here
      next->state = TASK_STATE_READY;
      next->forcefulWakeupTimeUnsafe = 0;
      // ^ is here to avoid interference with future statuses
      break;
    }
    next = next->next;
    if (!next) {
      fullRun++;
      if (fullRun > 2)
        break;
      next = firstTask;
    }
  }

  // found no task
  if (!next)
    next = dummyTask;

  Task *old = currentTask;

  currentTask = next;

  if (old->state != TASK_STATE_READY && old->spinlockQueueEntry) {
    // taskSpinlockExit(). maybe also todo on exit cleanup
    spinlockRelease(old->spinlockQueueEntry);
    old->spinlockQueueEntry = 0;
  }

  if (!next->kernel_task) {
    // per-process timers
    uint64_t rtAt = atomicRead64(&next->infoSignals->itimerReal.at);
    uint64_t rtReset = atomicRead64(&next->infoSignals->itimerReal.reset);
    if (rtAt && rtAt <= timerTicks) {
      // issue signal
      atomicBitmapSet(&next->sigPendingList, SIGALRM);
      if (!rtReset)
        atomicWrite64(&next->infoSignals->itimerReal.at, 0);
      else
        atomicWrite64(&next->infoSignals->itimerReal.at, timerTicks + rtReset);
    }
  }

#if SCHEDULE_DEBUG
  // if (old->id != 0 || next->id != 0)
  debugf("[scheduler] Switching context: id{%d} -> id{%d}\n", old->id,
         next->id);
  debugf("cpu->usermode_rsp{%lx} rip{%lx} fsbase{%lx} gsbase{%lx}\n",
         next->registers.usermode_rsp, next->registers.rip, old->fsbase,
         old->gsbase);
#endif

  // Before doing anything, handle any signals
  if (!next->kernel_task && !(next->registers.cs & GDT_KERNEL_CODE)) {
    signalsPendingHandleSched(next);
    if (next->state == TASK_STATE_SIGKILLED) { // killed in the process
      currentTask = old;
      return schedule(rsp);
    }
  }

  // Change TSS rsp0 (software multitasking)
  tssPtr->rsp0 = next->whileTssRsp;
  threadInfo.syscall_stack = next->whileSyscallRsp;

  // Save MSRIDs (HIGHLY unsure)
  // old->fsbase = rdmsr(MSRID_FSBASE);
  // old->gsbase = rdmsr(MSRID_GSBASE);

  // Apply new MSRIDs
  wrmsr(MSRID_FSBASE, next->fsbase);
  wrmsr(MSRID_GSBASE, next->gsbase);
  wrmsr(MSRID_KERNEL_GSBASE, (size_t)&threadInfo);

  // Save generic (and non) registers
  memcpy(&old->registers, cpu, sizeof(AsmPassedInterrupt));

  // Apply new generic (and non) registers (not needed!)
  // memcpy(cpu, &next->registers, sizeof(AsmPassedInterrupt));

  // Apply pagetable (not needed!)
  // ChangePageDirectoryUnsafe(next->pagedir);

  // Save & load appropriate FPU state
  if (!old->kernel_task) {
    asm volatile(" fxsave %0 " ::"m"(old->fpuenv));
    asm("stmxcsr (%%rax)" : : "a"(&old->mxcsr));
  }

  if (!next->kernel_task) {
    asm volatile(" fxrstor %0 " ::"m"(next->fpuenv));
    asm("ldmxcsr (%%rax)" : : "a"(&next->mxcsr));
  }

  // Cleanup any old tasks left dead (not needed!)
  // if (old->state == TASK_STATE_DEAD)
  //   taskKillCleanup(old);

  // Put next task's registers in tssRsp
  AsmPassedInterrupt *iretqRsp =
      (AsmPassedInterrupt *)(next->whileTssRsp - sizeof(AsmPassedInterrupt));
  memcpy(iretqRsp, &next->registers, sizeof(AsmPassedInterrupt));

  // Pass off control to our assembly finalization code that:
  //   - uses the tssRsp to iretq (give control back)
  //   - applies the new pagetable
  //   - cleanups old killed task (if necessary)
  // .. basically replaces all (not needed!) stuff
  uint64_t *pagedir =
      next->pagedirOverride ? next->pagedirOverride : next->infoPd->pagedir;
  ChangePageDirectoryFake(pagedir);
  // ^ just for globalPagedir to update (note potential race cond)
  asm_finalize((size_t)iretqRsp, VirtualToPhysical((size_t)pagedir));
}
