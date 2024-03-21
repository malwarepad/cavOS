#include <bootloader.h>
#include <gdt.h>
#include <isr.h>
#include <paging.h>
#include <schedule.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vmm.h>

// Clock-tick triggered scheduler
// Copyright (C) 2024 Panagiotis

#define SCHEDULE_DEBUG 0

extern TSSPtr *tssPtr;

// We need to "fix" (=ensure it's high enough) the RSP so the interrupt handler
// can do whatever it wants
uint8_t *genericStack = 0;
uint64_t rsp_fix(uint64_t rsp) {
  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;

  if (!genericStack) {
    genericStack = VirtualAllocate(10);
    memset(genericStack, 0, 10 * BLOCK_SIZE);
    genericStack -= sizeof(AsmPassedInterrupt);
  }

  memcpy(genericStack, cpu, sizeof(AsmPassedInterrupt));
  return genericStack;
}

void schedule(uint64_t rsp) {
  if (!tasksInitiated)
    return;

  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;
  Task               *next = currentTask->next;
  if (!next)
    next = firstTask;
  else {
    while (next->state != TASK_STATE_READY) {
      next = next->next;
      if (!next)
        next = firstTask;
    }
  }
  Task *old = currentTask;

  currentTask = next;

#if SCHEDULE_DEBUG
  // if (old->id != 0 || next->id != 0)
  debugf("[scheduler] Switching context: id{%d} -> id{%d}\n", old->id,
         next->id);
  debugf("cpu->usermode_rsp{%lx} rsp{%lx} fsbase{%lx} gsbase{%lx}\n",
         cpu->usermode_rsp, rsp, old->fsbase, old->gsbase);
#endif

  // Change TSS rsp0 (software multitasking)
  tssPtr->rsp0 = genericStack + sizeof(AsmPassedInterrupt);

  // Save MSRIDs
  old->fsbase = rdmsr(MSRID_FSBASE);
  old->gsbase = rdmsr(MSRID_GSBASE);

  // Apply new MSRIDs
  wrmsr(MSRID_FSBASE, next->fsbase);
  wrmsr(MSRID_GSBASE, next->gsbase);

  // Save generic (and non) registers
  memcpy(&old->registers, cpu, sizeof(AsmPassedInterrupt));

  // Apply new generic (and non) registers
  memcpy(cpu, &next->registers, sizeof(AsmPassedInterrupt));

  // Apply pagetable
  ChangePageDirectory(next->pagedir);
}
