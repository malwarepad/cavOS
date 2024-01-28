#include <gdt.h>
#include <isr.h>
#include <liballoc.h>
#include <paging.h>
#include <schedule.h>
#include <system.h>
#include <task.h>
#include <util.h>

// Task manager allowing for task management
// Copyright (C) 2023 Panagiotis

void create_task(uint32_t id, uint32_t eip, bool kernel_task,
                 uint32_t *pagedir) {
  lockInterrupts();

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

  Task *browse = firstTask;
  while (browse) {
    if (!browse->next)
      break; // found final
    browse = browse->next;
  }
  if (!browse) {
    debugf("[scheduler] Something went wrong with init!\n");
    panic();
  }
  Task *target = (Task *)malloc(sizeof(Task));
  memset(target, 0, sizeof(Task));
  browse->next = target;

  target->kesp_bottom = kernel_stack;
  target->kesp = (uint32_t)kesp;
  target->id = id;
  target->kernel_task = kernel_task;
  target->state = TASK_STATE_READY;
  target->pagedir = pagedir;

  target->heap_start = USER_HEAP_START;
  target->heap_end = USER_HEAP_START;

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

  Task *browse = firstTask;
  while (browse) {
    if (browse->next && (Task *)(browse->next)->id == id)
      break;
    browse = browse->next;
  }
  Task *task = browse->next;
  if (!task || task->state == TASK_STATE_DEAD)
    return;

  browse->next = task->next;

  uint32_t *kernel_stack = (uint32_t *)((task->kesp_bottom) + (0x1000 - 16));
  free(kernel_stack);

  // free user heap
  uint32_t *defaultPagedir = GetPageDirectory();
  ChangePageDirectory(task->pagedir);
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
  ChangePageDirectory(defaultPagedir);
  PageDirectoryFree(task->pagedir);

  // close any left open files
  OpenFile *file = task->firstFile;
  while (file) {
    OpenFile *curr = file;
    file = file->next;
    free(curr->dir);
    free(curr);
  }

  // funny workaround to save state somewhere
  memset(dummyTask, 0, sizeof(Task));
  if (currentTask == task)
    currentTask = dummyTask;
  free(task);

  schedule(); // go to the next task (will re-enable interrupts)
}

uint8_t *getTaskState(uint16_t id) {
  Task *browse = firstTask;
  while (browse) {
    if (browse->id == id)
      break;
    browse = browse->next;
  }
  if (!browse)
    return 0;

  return browse->state;
}

int16_t create_taskid() {
  Task    *browse = firstTask;
  uint16_t max = 0;
  while (browse) {
    max = browse->id;
    browse = browse->next;
  }

  return max + 1;
}

void initiateTasks() {
  dummyTask = (Task *)malloc(sizeof(Task));
  memset(dummyTask, 0, sizeof(Task));
  firstTask = (Task *)malloc(sizeof(Task));
  memset(firstTask, 0, sizeof(Task));

  currentTask = firstTask;
  currentTask->id = KERNEL_TASK;
  currentTask->state = TASK_STATE_READY;
  currentTask->pagedir = GetPageDirectory();
  currentTask->kernel_task = true;

  tasksInitiated = true;
  debugf("[tasks] Current execution ready for multitasking\n");

  // task 0 represents the execution we're in right now
}