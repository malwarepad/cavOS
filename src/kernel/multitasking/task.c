#include <gdt.h>
#include <isr.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <schedule.h>
#include <stack.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vmm.h>

// Task manager allowing for task management
// Copyright (C) 2024 Panagiotis

Task *taskCreate(uint32_t id, uint64_t rip, bool kernel_task, uint64_t *pagedir,
                 uint32_t argc, char **argv) {
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

  target->registers.ds = data_selector;
  target->registers.cs = code_selector;
  target->registers.usermode_ss = data_selector;
  target->registers.usermode_rsp = USER_STACK_BOTTOM;

  target->registers.rflags = 0x200; // enable interrupts
  target->registers.rip = rip;

  target->id = id;
  target->kernel_task = kernel_task;
  target->state = TASK_STATE_CREATED; // TASK_STATE_READY
  target->pagedir = pagedir;

  void  *tssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t tssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(tssRsp, 0, tssRspSize);
  target->tssRsp = (uint64_t)tssRsp + tssRspSize;

  target->heap_start = USER_HEAP_START;
  target->heap_end = USER_HEAP_START;

  return target;
}

Task *taskCreateKernel(uint64_t rip, uint64_t rdi) {
  Task *target =
      taskCreate(taskGenerateId(), rip, true, PageDirectoryAllocate(), 0, 0);
  stackGenerateKernel(target, rdi);
  taskCreateFinish(target);
  return target;
}

void taskCreateFinish(Task *task) { task->state = TASK_STATE_READY; }

void taskAdjustHeap(Task *task, size_t new_heap_end) {
  if (new_heap_end <= task->heap_start) {
    debugf("[task] Tried to adjust heap behind current values: id{%d}\n",
           task->id);
    taskKill(task->id);
    return;
  }

  size_t old_page_top = DivRoundUp(task->heap_end, PAGE_SIZE);
  size_t new_page_top = DivRoundUp(new_heap_end, PAGE_SIZE);

  if (new_page_top > old_page_top) {
    size_t num = new_page_top - old_page_top;

    for (size_t i = 0; i < num; i++) {
      size_t virt = old_page_top * PAGE_SIZE + i * PAGE_SIZE;
      if (VirtualToPhysical(virt))
        continue;

      size_t phys = BitmapAllocatePageframe(&physical);

      VirtualMap(virt, phys, PF_RW | PF_USER);

      memset((void *)virt, 0, PAGE_SIZE);
    }
  } else if (new_page_top < old_page_top) {
    debugf("[task] New page is lower than old page: id{%d}\n", task->id);
    taskKill(task->id);
    return;
  }

  task->heap_end = new_heap_end;
}

void taskKill(uint32_t id) {
  Task *task = taskGet(id);
  if (!task)
    return;
  task->state = TASK_STATE_DEAD;

  if (currentTask == task) {
    // we're most likely in a syscall context, so...
    // taskKillCleanup(task); // left for sched
    asm volatile("sti");
    // wait until we're outta here
    while (1) {
      //   debugf("GET ME OUT ");
    }
  }

  taskKillCleanup(task);
}

void taskKillCleanup(Task *task) {
  if (task->state != TASK_STATE_DEAD)
    return;

  Task *browse = firstTask;
  while (browse) {
    if (browse->next && browse->next->id == task->id)
      break;
    browse = browse->next;
  }
  // Task *task = browse->next;
  // if (!task || task->state == TASK_STATE_DEAD)
  //   return;

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

  // size_t currPagedir = (size_t)GetPageDirectory();
  // if (currPagedir == (size_t)task->pagedir)
  //   ChangePageDirectory(taskGet(KERNEL_TASK_ID)->pagedir);

  // PageDirectoryFree(task->pagedir); // left for sched

  // close any left open files
  Task *old = currentTask;
  currentTask = task;

  OpenFile *file = task->firstFile;
  while (file) {
    int id = file->id;
    file = file->next;
    fsUserClose(id);
  }

  SpecialFile *special = task->firstSpecialFile;
  while (special) {
    SpecialFile *next = special->next;
    fsUserCloseSpecial(special);
    special = next;
  }

  currentTask = old;

  PageDirectoryFree(task->pagedir);
  VirtualFree((void *)task->tssRsp, USER_STACK_PAGES);
  free(task);
}

Task *taskGet(uint32_t id) {
  Task *browse = firstTask;
  while (browse) {
    if (browse->id == id)
      break;
    browse = browse->next;
  }
  return browse;
}

uint8_t taskGetState(uint32_t id) {
  Task *browse = taskGet(id);
  if (!browse)
    return 0;

  return browse->state;
}

int16_t taskGenerateId() {
  Task    *browse = firstTask;
  uint16_t max = 0;
  while (browse) {
    if (browse->id > max)
      max = browse->id;
    browse = browse->next;
  }

  return max + 1;
}

bool taskChangeCwd(char *newdir) {
  stat_extra extra = {0};
  if (!fsStat(newdir, 0, &extra))
    return false;

  if (extra.file)
    return false;

  size_t len = strlength(newdir) + 1;
  currentTask->cwd = realloc(currentTask->cwd, len);
  memcpy(currentTask->cwd, newdir, len);
  return true;
}

void initiateTasks() {
  firstTask = (Task *)malloc(sizeof(Task));
  memset(firstTask, 0, sizeof(Task));

  currentTask = firstTask;
  currentTask->id = KERNEL_TASK_ID;
  currentTask->state = TASK_STATE_READY;
  currentTask->pagedir = GetPageDirectory();
  currentTask->kernel_task = true;

  void  *tssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t tssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(tssRsp, 0, tssRspSize);
  currentTask->tssRsp = (uint64_t)tssRsp + tssRspSize;

  debugf("[tasks] Current execution ready for multitasking\n");
  tasksInitiated = true;

  // task 0 represents the execution we're in right now
}