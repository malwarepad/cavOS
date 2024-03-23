#include <gdt.h>
#include <isr.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <schedule.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>

// Task manager allowing for task management
// Copyright (C) 2024 Panagiotis

// todo: move stack creation to this file, or have some way of controlling it
// for strictly kernel-only tasks, where ELF execution is not used!
Task *create_task(uint32_t id, uint64_t rip, bool kernel_task,
                  uint64_t *pagedir, uint32_t argc, char **argv) {
  lockInterrupts();

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

  uint64_t code_selector =
      kernel_task ? GDT_KERNEL_CODE : (GDT_USER_CODE | DPL_USER);
  uint64_t data_selector =
      kernel_task ? GDT_KERNEL_DATA : (GDT_USER_DATA | DPL_USER);

  // target->registers.cs = code_selector;
  target->registers.ds = data_selector;
  // target->registers.es = data_selector;
  // target->registers.fs = data_selector;
  // target->registers.gs = data_selector;

  // for (int i = 0; i < USER_STACK_PAGES; i++) {
  //   VirtualMap(USER_STACK_BOTTOM - USER_STACK_PAGES * 0x1000 + i * 0x1000,
  //              BitmapAllocatePageframe(),
  //              PF_SYSTEM | PF_USER | PF_RW);
  // }

  target->registers.cs = code_selector;
  target->registers.usermode_ss = data_selector;
  target->registers.usermode_rsp = USER_STACK_BOTTOM; // USER_STACK_BOTTOM

  target->registers.rflags = 0x200; // enable interrupts
  target->registers.rip = rip;

  // kesp -= sizeof(TaskReturnContext);
  // TaskReturnContext *context = (TaskReturnContext *)kesp;
  // context->edi = 0;
  // context->esi = 0;
  // context->ebx = 0;
  // context->ebp = 0;

  // this location gets read when returning from switch_context on a newly
  // created task, instead of going back through a bunch of functions we just
  // jump directly to isr_exit and exit the interrupt
  // context->return_eip = (uint64_t)asm_isr_exit;

  target->id = id;
  target->kernel_task = kernel_task;
  target->state = TASK_STATE_READY;
  target->pagedir = pagedir;

  target->heap_start = USER_HEAP_START;
  target->heap_end = USER_HEAP_START;

  // todo (when userspace tasks are ready)
  if (!kernel_task && argc) {
    // yeah, we will need to construct a stackframe...
    void *oldPagedir = GetPageDirectory();
    ChangePageDirectory(target->pagedir);

    // Store argument contents
    uint32_t argSpace = 0;
    for (int i = 0; i < argc; i++)
      argSpace += strlength(argv[i]) + 1; // null terminator
    uint8_t *argStart = target->heap_end;
    adjust_user_heap(target, target->heap_end + argSpace);
    size_t ellapsed = 0;
    for (int i = 0; i < argc; i++) {
      uint32_t len = strlength(argv[i]) + 1; // null terminator
      memcpy((size_t)argStart + ellapsed, argv[i], len);
      ellapsed += len;
    }

    // todo: Proper environ
    uint64_t *environStart = target->heap_end;
    adjust_user_heap(target, target->heap_end + sizeof(uint64_t) * 10);
    memset(environStart, 0, sizeof(uint64_t) * 10);
    environStart[0] = &environStart[5];

    // Reserve stack space for environ
    target->registers.usermode_rsp -= sizeof(uint64_t);
    uint64_t *finalEnviron = target->registers.usermode_rsp;

    // Store argument pointers (directly in stack)
    size_t finalEllapsed = 0;
    // ellapsed already has the full size lol
    for (int i = argc - 1; i >= 0; i--) {
      target->registers.usermode_rsp -= sizeof(uint64_t);
      uint64_t *finalArgv = target->registers.usermode_rsp;

      uint32_t len = strlength(argv[i]) + 1; // null terminator
      finalEllapsed += len;
      *finalArgv = (size_t)argStart + (ellapsed - finalEllapsed);
    }

    // Reserve stack space for argc
    target->registers.usermode_rsp -= sizeof(uint64_t);
    uint64_t *finalArgc = target->registers.usermode_rsp;

    // Put everything left in the stack, as expected
    *finalArgc = argc;
    *finalEnviron = finalEnviron;

    ChangePageDirectory(oldPagedir);
  }

  releaseInterrupts();

  return target;
}

void adjust_user_heap(Task *task, size_t new_heap_end) {
  if (new_heap_end <= task->heap_start) {
    debugf("[task] Tried to adjust heap behind current values: id{%d}\n",
           task->id);
    kill_task(task->id);
    return;
  }

  size_t old_page_top = DivRoundUp(task->heap_end, PAGE_SIZE);
  size_t new_page_top = DivRoundUp(new_heap_end, PAGE_SIZE);

  if (new_page_top > old_page_top) {
    size_t num = new_page_top - old_page_top;

    for (size_t i = 0; i < num; i++) {
      size_t phys = BitmapAllocatePageframe(&physical);
      size_t virt = old_page_top * PAGE_SIZE + i * PAGE_SIZE;
      if (VirtualToPhysical(virt))
        continue;

      VirtualMap(virt, phys, PF_RW | PF_USER | PF_SYSTEM);

      memset((void *)virt, 0, PAGE_SIZE);
    }
  } else if (new_page_top < old_page_top) {
    debugf("[task] New page is lower than old page: id{%d}\n", task->id);
    kill_task(task->id);
    return;
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

  // free user heap
  /*uint32_t *defaultPagedir = GetPageDirectory();
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
  ChangePageDirectory(defaultPagedir);*/
  // ^ not needed because of PageDirectoryFree() doing it automatically

  uint64_t currPagedir = GetPageDirectory();
  if (currPagedir == task->pagedir)
    ChangePageDirectory(getTask(KERNEL_TASK)->pagedir);

  // PageDirectoryFree(task->pagedir); // left for sched

  // close any left open files
  OpenFile *file = task->firstFile;
  Task     *old = currentTask;
  currentTask = task;
  while (file) {
    int id = file->id;
    file = file->next;
    fsUserClose(id);
  }
  currentTask = old;

  // funny workaround to save state somewhere
  // memset(dummyTask, 0, sizeof(Task));
  // if (currentTask == task)
  //   currentTask = dummyTask;

  // free(task); // left for sched

  // task->state = TASK_STATE_DEAD;
  // releaseInterrupts();

  asm volatile("sti");
  // wait until we're outta here
  while (1) {
  }
}

void killed_task_cleanup(Task *task) {
  PageDirectoryFree(task->pagedir);
  free(task);
}

uint8_t *getTaskState(uint32_t id) {
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

Task *getTask(uint32_t id) {
  Task *browse = firstTask;
  while (browse) {
    if (browse->id == id)
      break;
    browse = browse->next;
  }
  return browse;
}

int16_t create_taskid() {
  Task    *browse = firstTask;
  uint16_t max = 0;
  while (browse) {
    if (browse->id > max)
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