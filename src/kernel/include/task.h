#include "fs_controller.h"
#include "isr.h"
#include "types.h"

#ifndef TASK_H
#define TASK_H

// "Descriptor Privilege Level"
#define DPL_USER 3

#define KERNEL_TASK_ID 0

typedef struct {
  uint64_t edi;
  uint64_t esi;
  uint64_t ebx;
  uint64_t ebp;
  uint64_t return_eip;
} TaskReturnContext;

typedef enum TASK_STATE {
  TASK_STATE_DEAD = 0,
  TASK_STATE_READY = 1,
  TASK_STATE_IDLE = 2,
  TASK_STATE_WAITING_INPUT = 3,
  TASK_STATE_CREATED = 4, // just made by taskCreate()
} TASK_STATE;

typedef struct Task Task;

struct Task {
  uint64_t id;
  bool     kernel_task;
  uint8_t  state;

  AsmPassedInterrupt registers;
  uint64_t          *pagedir;
  uint64_t           tssRsp;

  bool systemCallInProgress;

  // Useful to switch, for when TLS is available
  uint64_t fsbase;
  uint64_t gsbase;

  uint64_t heap_start;
  uint64_t heap_end;

  uint32_t  tmpRecV;
  OpenFile *firstFile;
  char     *cwd;

  __attribute__((aligned(16))) uint8_t fpuenv[512];

  Task *next;
};

Task *firstTask;
Task *currentTask;

bool tasksInitiated;

void  initiateTasks();
Task *taskCreate(uint32_t id, uint64_t rip, bool kernel_task, uint64_t *pagedir,
                 uint32_t argc, char **argv);
Task *taskCreateKernel(uint64_t rip, uint64_t rdi);
void  taskCreateFinish(Task *task);
void  taskAdjustHeap(Task *task, size_t new_heap_end);
void  taskKill(uint32_t id);
void  taskKillCleanup(Task *task);
uint8_t taskGetState(uint32_t id);
Task   *taskGet(uint32_t id);
int16_t taskGenerateId();

#endif
