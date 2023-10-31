#include "../../include/task.h"
#include "../../include/liballoc.h"
#include "../../include/schedule.h"

// Task manager allowing for task management
// Copyright (C) 2023 Panagiotis

void create_task(uint32_t id, uint32_t eip, bool kernel_task,
                 uint32_t *pagedir) {
  taskSwitchSpinlock = true;

  memset(&tasks[id], 0, sizeof(Task));

  // when a task gets context switched to for the first time,
  // switch_context is going to start popping values from the stack into
  // registers, so we need to set up a predictable stack frame for that

  uint32_t kernel_stack = (uint32_t)malloc(0x1000 - 16) + 0x1000 - 16;
  uint8_t *kesp = (uint8_t *)kernel_stack;

  kesp -= sizeof(AsmPassedInterrupt);
  AsmPassedInterrupt *trap = (AsmPassedInterrupt *)kesp;
  memset((uint8_t *)trap, 0, sizeof(AsmPassedInterrupt));

  uint32_t code_selector =
      kernel_task ? GDT_KERNEL_CODE : (GDT_USER_CODE | RPL_USER);
  uint32_t data_selector =
      kernel_task ? GDT_KERNEL_DATA : (GDT_USER_DATA | RPL_USER);

  trap->cs = code_selector;
  trap->ds = data_selector;
  trap->es = data_selector;
  trap->fs = data_selector;
  trap->gs = data_selector;
  // for (int i = 0; i < USER_STACK_PAGES; i++) {
  //   VirtualMap(USER_STACK_BOTTOM - USER_STACK_PAGES * 0x1000 + i * 0x1000,
  //              BitmapAllocatePageframe(),
  //              PAGE_FLAG_OWNER | PAGE_FLAG_USER | PAGE_FLAG_WRITE);
  // }

  trap->usermode_ss = data_selector;
  trap->usermode_esp = USER_STACK_BOTTOM;

  trap->eflags = 0x200; // enable interrupts
  trap->eip = eip;

  kesp -= sizeof(TaskReturnContext);
  TaskReturnContext *context = (TaskReturnContext *)kesp;
  context->edi = 0;
  context->esi = 0;
  context->ebx = 0;
  context->ebp = 0;

  // this location gets read when returning from switch_context on a newly
  // created task, instead of going back through a bunch of functions we just
  // jump directly to isr_exit and exit the interrupt
  context->return_eip = (uint32_t)asm_isr_exit;

  tasks[id].kesp_bottom = kernel_stack;
  tasks[id].kesp = (uint32_t)kesp;
  tasks[id].id = id;
  tasks[id].kernel_task = kernel_task;
  tasks[id].state = TASK_STATE_READY;
  tasks[id].pagedir = pagedir;

  taskSwitchSpinlock = false;
}

void kill_task(uint32_t id) {
  taskSwitchSpinlock = true;

  Task *task = &tasks[id];
  if (task->state == TASK_STATE_DEAD)
    return;

  PageDirectoryFree(task->pagedir);
  uint32_t *kernel_stack = (uint32_t *)((task->kesp_bottom) + (0x1000 - 16));
  free(kernel_stack);
  memset(&tasks[id], 0, sizeof(Task));

  // task->state = TASK_STATE_DEAD;
  // num_tasks--;

  taskSwitchSpinlock = false;
  schedule(); // go to the next task
}

void initiateTasks() {
  tasksInitiated = true;
  taskSwitchSpinlock = false;
  memset((uint8_t *)tasks, 0, sizeof(Task) * MAX_TASKS);

  current_task = &tasks[KERNEL_TASK];
  current_task->id = KERNEL_TASK;
  current_task->state = TASK_STATE_READY;
  current_task->pagedir = GetPageDirectory();

  // task 0 represents the execution we're in right now
}