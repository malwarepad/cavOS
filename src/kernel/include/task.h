#include "fat32.h"
#include "types.h"

#ifndef TASK_H
#define TASK_H

// "requested privilage level"
#define RPL_USER 3

#define KERNEL_TASK 0

// matches the stack of switch_context in kernel.asm
typedef struct {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebx;
  uint32_t ebp;
  uint32_t return_eip;
} TaskReturnContext;

#define TASK_STATE_DEAD 0
#define TASK_STATE_READY 1
#define TASK_STATE_IDLE 2

typedef struct Task Task;

struct Task {
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

  uint32_t *pagedir;
  bool      kernel_task;
  uint8_t   state;

  uint32_t heap_start;
  uint32_t heap_end;

  OpenFile *firstFile;

  Task *next;
};

Task *dummyTask;
Task *firstTask;
Task *currentTask;

bool tasksInitiated;

void     initiateTasks();
void     create_task(uint32_t id, uint32_t eip, bool kernel_task,
                     uint32_t *pagedir);
void     adjust_user_heap(Task *task, uint32_t new_heap_end);
void     kill_task(uint32_t id);
uint8_t *getTaskState(uint16_t id);

#endif
