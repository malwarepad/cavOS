#include <gdt.h>
#include <isr.h>
#include <liballoc.h>
#include <paging.h>
#include <schedule.h>
#include <system.h>
#include <task.h>

// Task manager allowing for task management
// Copyright (C) 2023 Panagiotis

void create_task(uint32_t id, uint32_t eip, bool kernel_task,
                 uint32_t *pagedir) {
  lockInterrupts();

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

  tasks[id].heap_start = USER_HEAP_START;
  tasks[id].heap_end = USER_HEAP_START;

  releaseInterrupts();
}

void adjust_user_heap(Task *task, uint32_t new_heap_end) {
  if (new_heap_end <= task->heap_start) {
    debugf("[task] Tried to adjust heap behind current values: id{%d}\n",
           task->id);
    return;
  }

  int old_page_top = DivRoundUp(task->heap_end, PAGE_SIZE);
  int new_page_top = DivRoundUp(new_heap_end, PAGE_SIZE);

  if (new_page_top > old_page_top) {
    int num = new_page_top - old_page_top;

    for (int i = 0; i < num; i++) {
      uint32_t phys = BitmapAllocatePageframe(&physical);
      uint32_t virt = old_page_top * PAGE_SIZE + i * PAGE_SIZE;

      VirtualMap(virt, phys,
                 PAGE_FLAG_WRITE | PAGE_FLAG_USER | PAGE_FLAG_OWNER);

      memset((void *)virt, 0, PAGE_SIZE);
    }
  } else if (new_page_top < old_page_top) {
    debugf("[task] New page is lower than old page: id{%d}\n", task->id);
    // optional:
    // printf("[task] New page is lower than old page: id{%d}\n", task->id);
    panic();
  }

  task->heap_end = new_heap_end;
}

void kill_task(uint32_t id) {
  lockInterrupts();

  Task *task = &tasks[id];
  if (task->state == TASK_STATE_DEAD)
    return;

  PageDirectoryFree(task->pagedir);
  uint32_t *kernel_stack = (uint32_t *)((task->kesp_bottom) + (0x1000 - 16));
  free(kernel_stack);
  memset(&tasks[id], 0, sizeof(Task));

  // free user heap
  int heap_start = DivRoundUp(task->heap_start, PAGE_SIZE);
  int heap_end = DivRoundUp(task->heap_end, PAGE_SIZE);

  if (heap_end > heap_start) {
    int num = heap_end - heap_start;

    for (int i = 0; i < num; i++) {
      uint32_t virt = heap_start * PAGE_SIZE + i * PAGE_SIZE;
      uint32_t phys = VirtualToPhysical(virt);
      BitmapFreePageframe(&physical, phys);
    }
  }

  // task->state = TASK_STATE_DEAD;
  // num_tasks--;

  schedule(); // go to the next task (will re-enable interrupts)
}

int16_t create_taskid() {
  for (int i = 1; i < MAX_TASKS; i++) {
    if (tasks[i].state != TASK_STATE_IDLE && tasks[i].state != TASK_STATE_READY)
      return i;
  }

  return -1;
}

void initiateTasks() {
  tasksInitiated = true;
  memset((uint8_t *)tasks, 0, sizeof(Task) * MAX_TASKS);

  current_task = &tasks[KERNEL_TASK];
  current_task->id = KERNEL_TASK;
  current_task->state = TASK_STATE_READY;
  current_task->pagedir = GetPageDirectory();
  current_task->kernel_task = true;

  debugf("[tasks] Current execution ready for multitasking\n");

  // task 0 represents the execution we're in right now
}