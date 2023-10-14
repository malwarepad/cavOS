#include "gdt.h"
#include "isr.h"
#include "types.h"

#ifndef TASK_H
#define TASK_H

// "requested privilage level"
#define RPL_USER 3

// fixed number of tasks for simplicity
#define MAX_TASKS 16

// matches the stack of switch_context in kernel.asm
typedef struct {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebx;
  uint32_t ebp;
  uint32_t return_eip;
} TaskReturnContext;

typedef struct {
  uint32_t id;

  // each task has its own kernel stack
  //  this stack gets loaded on interrupts
  //  when context switching between two tasks,
  //  this stack is used to store the state of the registers etc.

  // we could also have stored them here though

  // kernel stack pointer, updated when switching contexts
  //  to switch to this task, we load this into esp and pop the state
  uint32_t kesp;

  // bottom(highest address) of kernel stack
  //  esp gets set to this via the TSS when transitioning from user to kernel
  //  mode on interrupts, so we set it to the bottom of this task's kernel stack
  //  address to get an empty stack. this is only used in user tasks.
  uint32_t kesp_bottom;
} Task;

Task  tasks[MAX_TASKS];
int   num_tasks;
Task *current_task;

void setup_tasks();
void create_task(uint32_t id, uint32_t eip, uint32_t user_stack,
                 uint32_t kernel_stack, bool kernel_task);

#endif
