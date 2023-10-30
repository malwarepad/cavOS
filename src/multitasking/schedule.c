#include "../../include/schedule.h"
#include "../../include/task.h"
#include "../../include/util.h"

// Clock-tick triggered scheduler
// Copyright (C) 2023 Panagiotis

#define SCHEDULE_DEBUG 0

void schedule() {
  if (num_tasks < 2 || taskSwitchSpinlock)
    return;

  int next_id = (current_task->id + 1) % num_tasks;

  Task *next = &tasks[next_id];
  Task *old = current_task;
  current_task = next;

  // update tss
  tss.esp0 = next->kesp_bottom;

#if SCHEDULE_DEBUG
  debugf("switching context from %d to %d\n", old->id, next->id);
#endif
  // switch context, may not return here
  switch_context(old, next);
#if SCHEDULE_DEBUG
  debugf("switched context from %d to %d\n", old->id, next->id);
#endif
}
