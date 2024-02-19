#include <gdt.h>
#include <isr.h>
#include <schedule.h>
#include <task.h>
#include <util.h>

// Clock-tick triggered scheduler
// Copyright (C) 2024 Panagiotis

#define SCHEDULE_DEBUG 0

void schedule() {
  if (!tasksInitiated)
    return;

  Task *next = currentTask->next;
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

  // update tss
  update_tss_esp0(next->kesp_bottom);

#if SCHEDULE_DEBUG
  // if (old->id != 0 || next->id != 0)
  debugf("[scheduler] Switching context: id{%d} -> id{%d}\n", old->id,
         next->id);
#endif
  // switch context, may not return here
  switch_context(old, next);
  // do NOT reprint or it might pagefault
}
