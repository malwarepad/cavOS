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

  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;
  Task *next = currentTask->next ? currentTask->next : firstTask;
  int fullRun = 0;

  // Fast path: if next is ready, skip loop
  while (next->state != TASK_STATE_READY) {
    if (signalsRevivableState(next->state) && signalsPendingQuick(next)) {
      assert(next->registers.cs & GDT_KERNEL_CODE);
      next->forcefulWakeupTimeUnsafe = 0;
      next->state = TASK_STATE_READY;
      break;
    }
    if (next->forcefulWakeupTimeUnsafe && next->forcefulWakeupTimeUnsafe <= timerTicks) {
      next->state = TASK_STATE_READY;
      next->forcefulWakeupTimeUnsafe = 0;
      break;
    }
    next = next->next ? next->next : firstTask;
    if (++fullRun > 2) break;
  }

  if (!next) next = dummyTask;
  Task *old = currentTask;
  currentTask = next;

  if (old->state != TASK_STATE_READY && old->spinlockQueueEntry) {
    spinlockRelease(old->spinlockQueueEntry);
    old->spinlockQueueEntry = 0;
  }

  if (!next->kernel_task) {
    // Optimize timer checks: only read atomic values once
    struct {
      uint64_t at, reset;
    } itimerReal = {
      .at = atomicRead64(&next->infoSignals->itimerReal.at),
      .reset = atomicRead64(&next->infoSignals->itimerReal.reset)
    };
    if (itimerReal.at && itimerReal.at <= timerTicks) {
      atomicBitmapSet(&next->sigPendingList, SIGALRM);
      atomicWrite64(&next->infoSignals->itimerReal.at,
        itimerReal.reset ? timerTicks + itimerReal.reset : 0);
    }
  }

#if SCHEDULE_DEBUG
  debugf("[scheduler] Switching context: id{%d} -> id{%d}\n", old->id, next->id);
  debugf("cpu->usermode_rsp{%lx} rip{%lx} fsbase{%lx} gsbase{%lx}\n",
         next->registers.usermode_rsp, next->registers.rip, old->fsbase, old->gsbase);
#endif

  // Handle signals before context switch
  if (!next->kernel_task && !(next->registers.cs & GDT_KERNEL_CODE)) {
    signalsPendingHandleSched(next);
    if (next->state == TASK_STATE_SIGKILLED) {
      currentTask = old;
      return schedule(rsp);
    }
  }

  // Change TSS rsp0 and syscall stack
  tssPtr->rsp0 = next->whileTssRsp;
  threadInfo.syscall_stack = next->whileSyscallRsp;

  // Apply new MSRIDs
  wrmsr(MSRID_FSBASE, next->fsbase);
  wrmsr(MSRID_GSBASE, next->gsbase);
  wrmsr(MSRID_KERNEL_GSBASE, (size_t)&threadInfo);

  // Save registers
  memcpy(&old->registers, cpu, sizeof(AsmPassedInterrupt));

  // Save/load FPU state only for user tasks
  if (!old->kernel_task) {
    asm volatile(" fxsave %0 " ::"m"(old->fpuenv));
    asm("stmxcsr (%%rax)" : : "a"(&old->mxcsr));
  }
  if (!next->kernel_task) {
    asm volatile(" fxrstor %0 " ::"m"(next->fpuenv));
    asm("ldmxcsr (%%rax)" : : "a"(&next->mxcsr));
  }

  // Prepare iretq stack
  AsmPassedInterrupt *iretqRsp =
      (AsmPassedInterrupt *)(next->whileTssRsp - sizeof(AsmPassedInterrupt));
  memcpy(iretqRsp, &next->registers, sizeof(AsmPassedInterrupt));

  // Update pagedir pointer (no full switch, just update global)
  uint64_t *pagedir = next->pagedirOverride ? next->pagedirOverride : next->infoPd->pagedir;
  ChangePageDirectoryFake(pagedir);

  // Finalize context switch
  asm_finalize((size_t)iretqRsp, VirtualToPhysical((size_t)pagedir));
}
