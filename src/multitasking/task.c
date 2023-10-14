#include "../../include/task.h"

// Task manager allowing for task management
// Copyright (C) 2023 Panagiotis

void create_task(uint32_t id, uint32_t eip, uint32_t user_stack,
                 uint32_t kernel_stack, bool kernel_task) {
  num_tasks++;

  // when a task gets context switched to for the first time,
  // switch_context is going to start popping values from the stack into
  // registers, so we need to set up a predictable stack frame for that

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

  trap->usermode_ss = data_selector;
  trap->usermode_esp = user_stack;

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
}

void setup_tasks() {
  memset((uint8_t *)tasks, 0, sizeof(Task) * MAX_TASKS);

  num_tasks = 1;
  current_task = &tasks[0];
  current_task->id = 0;

  // task 0 represents the execution we're in right now
}