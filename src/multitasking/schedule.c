#include "../../include/schedule.h"
#include "../../include/util.h"

// Clock-tick triggered scheduler
// Copyright (C) 2023 Panagiotis

void schedule() {
  if (num_tasks > 0) {
    int next_id = (current_task->id + 1) % num_tasks;

    Task *next = &tasks[next_id];
    Task *old = current_task;
    current_task = next;

    // update tss
    tss.esp0 = next->kesp_bottom;

    // switch context, may not return here
    switch_context(old, next);
  }
}
