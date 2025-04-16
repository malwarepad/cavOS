#include <gdt.h>
#include <isr.h>
#include <kernel_helper.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <schedule.h>
#include <stack.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>
#include <vmm.h>

// Task manager allowing for task management
// Copyright (C) 2024 Panagiotis

SpinlockCnt TASK_LL_MODIFY = {0};

void taskAttachDefTermios(Task *task) {
  memset(&task->term, 0, sizeof(termios));
  task->term.c_iflag = BRKINT | ICRNL | INPCK | ISTRIP | IXON;
  task->term.c_oflag = OPOST;
  task->term.c_cflag = CS8 | CREAD | CLOCAL;
  task->term.c_lflag = ECHO | ICANON | IEXTEN | ISIG;
  task->term.c_line = 0;
  task->term.c_cc[VINTR] = 3;     // Ctrl-C
  task->term.c_cc[VQUIT] = 28;    // Ctrl-task->term.c_cc[VERASE] = 127; // DEL
  task->term.c_cc[VKILL] = 21;    // Ctrl-U
  task->term.c_cc[VEOF] = 4;      // Ctrl-D
  task->term.c_cc[VTIME] = 0;     // No timer
  task->term.c_cc[VMIN] = 1;      // Return each byte
  task->term.c_cc[VSTART] = 17;   // Ctrl-Q
  task->term.c_cc[VSTOP] = 19;    // Ctrl-S
  task->term.c_cc[VSUSP] = 26;    // Ctrl-Z
  task->term.c_cc[VREPRINT] = 18; // Ctrl-R
  task->term.c_cc[VDISCARD] = 15; // Ctrl-O
  task->term.c_cc[VWERASE] = 23;  // Ctrl-W
  task->term.c_cc[VLNEXT] = 22;   // Ctrl-V
  // Initialize other control characters to 0
  for (int i = 16; i < NCCS; i++) {
    task->term.c_cc[i] = 0;
  }
}

Task *taskCreate(uint32_t id, uint64_t rip, bool kernel_task, uint64_t *pagedir,
                 uint32_t argc, char **argv) {
  spinlockCntWriteAcquire(&TASK_LL_MODIFY);
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
  spinlockCntWriteRelease(&TASK_LL_MODIFY);

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
  target->tgid = id;
  target->kernel_task = kernel_task;
  target->state = TASK_STATE_CREATED; // TASK_STATE_READY
  // target->pagedir = pagedir;
  target->infoPd = taskInfoPdAllocate(false);
  target->infoPd->pagedir = pagedir; // no lock cause only we use it

  void  *tssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t tssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(tssRsp, 0, tssRspSize);
  target->whileTssRsp = (uint64_t)tssRsp + tssRspSize;

  void  *syscalltssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t syscalltssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(syscalltssRsp, 0, syscalltssRspSize);
  target->whileSyscallRsp = (uint64_t)syscalltssRsp + syscalltssRspSize;

  target->infoFs = taskInfoFsAllocate();
  target->infoFiles = taskInfoFilesAllocate();
  target->infoSignals = taskInfoSignalAllocate();

  memset(target->fpuenv, 0, 512);
  ((uint16_t *)target->fpuenv)[0] = 0x37f;
  target->mxcsr = 0x1f80;

  taskAttachDefTermios(target);

  // just in case it ends up becoming an orphan
  target->parent = firstTask;

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

void taskAdjustHeap(Task *task, size_t new_heap_end, size_t *start,
                    size_t *end) {
  if (new_heap_end <= *start) {
    debugf("[task] Tried to adjust heap behind current values: id{%d}\n",
           task->id);
    taskKill(task->id, 139);
    return;
  }

  size_t old_page_top = DivRoundUp(*end, PAGE_SIZE);
  size_t new_page_top = DivRoundUp(new_heap_end, PAGE_SIZE);

  if (new_page_top > old_page_top) {
    size_t num = new_page_top - old_page_top;

    for (size_t i = 0; i < num; i++) {
      size_t virt = old_page_top * PAGE_SIZE + i * PAGE_SIZE;
      if (VirtualToPhysical(virt))
        continue;

      size_t phys = PhysicalAllocate(1);

      VirtualMap(virt, phys, PF_RW | PF_USER);

      memset((void *)virt, 0, PAGE_SIZE);
    }
  } else if (new_page_top < old_page_top) {
    debugf("[task] New page is lower than old page: id{%d}\n", task->id);
    taskKill(task->id, 139);
    return;
  }

  *end = new_heap_end;
}

void taskCallReaper(Task *target) {
  while (true) {
    spinlockAcquire(&LOCK_REAPER);
    if (!reaperTask) {
      // there is space!
      reaperTask = target;
      spinlockRelease(&LOCK_REAPER);
      return;
    }
    spinlockRelease(&LOCK_REAPER);
  }
}

void taskKill(uint32_t id, uint16_t ret) {
  Task *task = taskGet(id);
  if (!task)
    return;

  // We'll need this later
  // bool parentVfork = task->parent->state == TASK_STATE_WAITING_VFORK;

  // Notify that poor parent... they must've been searching all over the
  // place!
  if (task->parent && !task->noInformParent) {
    spinlockAcquire(&task->parent->LOCK_CHILD_TERM);
    KilledInfo *info = (KilledInfo *)LinkedListAllocate(
        (void **)(&task->parent->firstChildTerminated), sizeof(KilledInfo));
    info->pid = task->id;
    info->ret = ret;
    task->parent->childrenTerminatedAmnt++;
    if (task->parent->state == TASK_STATE_WAITING_CHILD ||
        (task->parent->state == TASK_STATE_WAITING_CHILD_SPECIFIC &&
         task->parent->waitingForPid == task->id))
      task->parent->state = TASK_STATE_READY;
    spinlockRelease(&task->parent->LOCK_CHILD_TERM);
    atomicBitmapSet(&task->parent->sigPendingList, SIGCHLD);
  }

  // vfork() children need to notify parents no matter what
  if (task->parent->state == TASK_STATE_WAITING_VFORK)
    task->parent->state = TASK_STATE_READY;

  if (task->tidptr)
    *task->tidptr = 0; // todo: futex wakeup

  // close any left open files
  taskInfoFilesDiscard(task->infoFiles, task);

  // if (!parentVfork)
  //   PageDirectoryFree(task->pagedir);
  taskInfoPdDiscard(task->infoPd);
  // ^ only changes userspace locations so we don't need to change our pagedir

  // the "reaper" thread will finish everything in a safe context
  taskCallReaper(task);
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
}

void taskFreeChildren(Task *task) {
  if (task->noInformParent)
    return; // it's execve() trash most likely
  Task *child = firstTask;
  while (child) {
    Task *next = child->next;
    // todo: reparent to init!
    if (child->parent == task && child->state != TASK_STATE_DEAD) {
      if (!child->parent)
        child->parent = firstTask;
      else
        child->parent = child->parent->parent;
    } // ykyk
    child = next;
  }
}

void taskKillChildren(Task *task) {
  Task *child = firstTask;
  while (child) {
    Task *next = child->next;
    if (child->parent == task && child->state != TASK_STATE_DEAD) {
      taskKill(child->id, 0);
      // taskKillCleanup(child); // done automatically
      taskKillChildren(task); // use recursion
    }
    child = next;
  }
}

Task *taskGet(uint32_t id) {
  spinlockCntReadAcquire(&TASK_LL_MODIFY);
  Task *browse = firstTask;
  while (browse) {
    if (browse->id == id)
      break;
    browse = browse->next;
  }
  spinlockCntReadRelease(&TASK_LL_MODIFY);
  return browse;
}

int taskIdCurr = 1;

int16_t taskGenerateId() {
  return taskIdCurr++;
  // spinlockCntReadAcquire(&TASK_LL_MODIFY);
  // Task    *browse = firstTask;
  // uint16_t max = 0;
  // while (browse) {
  //   if (browse->id > max)
  //     max = browse->id;
  //   browse = browse->next;
  // }

  // spinlockCntReadRelease(&TASK_LL_MODIFY);
  // return max + 1;
}

size_t taskChangeCwd(char *newdir) {
  stat stat = {0};
  spinlockAcquire(&currentTask->infoFs->LOCK_FS);
  char *safeNewdir = fsSanitize(currentTask->infoFs->cwd, newdir);
  spinlockRelease(&currentTask->infoFs->LOCK_FS);
  if (!fsStatByFilename(currentTask, safeNewdir, &stat)) {
    free(safeNewdir);
    return ERR(ENOENT);
  }

  if (!(stat.st_mode & S_IFDIR)) {
    free(safeNewdir);
    return ERR(ENOTDIR);
  }

  size_t len = strlength(safeNewdir) + 1;
  spinlockAcquire(&currentTask->infoFs->LOCK_FS);
  currentTask->infoFs->cwd = realloc(currentTask->infoFs->cwd, len);
  memcpy(currentTask->infoFs->cwd, safeNewdir, len);
  spinlockRelease(&currentTask->infoFs->LOCK_FS);

  free(safeNewdir);
  return 0;
}

void taskFilesCopy(Task *original, Task *target, bool respectCOE) {
  TaskInfoFiles *originalInfo = original->infoFiles;
  TaskInfoFiles *targetInfo = target->infoFiles;
  spinlockCntReadAcquire(&originalInfo->WLOCK_FILES);
  spinlockCntWriteAcquire(&targetInfo->WLOCK_FILES);
  targetInfo->rlimitFdsHard = originalInfo->rlimitFdsHard;
  targetInfo->rlimitFdsSoft = originalInfo->rlimitFdsSoft;
  targetInfo->fdBitmap = malloc(targetInfo->rlimitFdsHard / 8);
  memcpy(targetInfo->fdBitmap, originalInfo->fdBitmap,
         targetInfo->rlimitFdsHard / 8);
  OpenFile *realFile = originalInfo->firstFile;
  while (realFile) {
    if (respectCOE && realFile->closeOnExec) {
      realFile = realFile->next;
      continue;
    }
    OpenFile *targetFile = fsUserDuplicateNodeUnsafe(realFile);
    LinkedListPushFrontUnsafe((void **)(&targetInfo->firstFile), targetFile);
    realFile = realFile->next;
  }
  spinlockCntWriteRelease(&targetInfo->WLOCK_FILES);
  spinlockCntReadRelease(&originalInfo->WLOCK_FILES);
}

Task *taskFork(AsmPassedInterrupt *cpu, uint64_t rsp, int cloneFlags,
               bool spinup) {
  spinlockCntWriteAcquire(&TASK_LL_MODIFY);
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
  spinlockCntWriteRelease(&TASK_LL_MODIFY);

  if (!(cloneFlags & CLONE_VM)) {
    target->infoPd = taskInfoPdClone(currentTask->infoPd);
  } else {
    TaskInfoPagedir *share = currentTask->infoPd;
    spinlockAcquire(&share->LOCK_PD);
    share->utilizedBy++;
    spinlockRelease(&share->LOCK_PD);
    target->infoPd = share; // share it yk!
  }

  target->id = taskGenerateId();
  target->tgid = target->id;
  target->pgid = currentTask->pgid;
  target->kernel_task = currentTask->kernel_task;
  target->state = TASK_STATE_CREATED;

  if (cloneFlags & CLONE_THREAD)
    target->tgid = currentTask->tgid;

  // target->registers = currentTask->registers;
  memcpy(&target->registers, cpu, sizeof(AsmPassedInterrupt));
  void  *tssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t tssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(tssRsp, 0, tssRspSize);
  target->whileTssRsp = (uint64_t)tssRsp + tssRspSize;

  void  *syscalltssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t syscalltssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(syscalltssRsp, 0, syscalltssRspSize);
  target->whileSyscallRsp = (uint64_t)syscalltssRsp + syscalltssRspSize;

  target->fsbase = currentTask->fsbase;
  target->gsbase = currentTask->gsbase;

  // target->heap_start = currentTask->heap_start;
  // target->heap_end = currentTask->heap_end;

  // target->mmap_start = currentTask->mmap_start;
  // target->mmap_end = currentTask->mmap_end;

  target->term = currentTask->term;

  target->tmpRecV = currentTask->tmpRecV;

  if (!(cloneFlags & CLONE_FS))
    target->infoFs = taskInfoFsClone(currentTask->infoFs);
  else {
    TaskInfoFs *share = currentTask->infoFs;
    spinlockAcquire(&share->LOCK_FS);
    share->utilizedBy++;
    spinlockRelease(&share->LOCK_FS);
    target->infoFs = share;
  }

  if (!(cloneFlags & CLONE_FILES)) {
    target->infoFiles = taskInfoFilesAllocate();
    taskFilesCopy(currentTask, target, false);
  } else {
    TaskInfoFiles *share = currentTask->infoFiles;
    spinlockCntWriteAcquire(&share->WLOCK_FILES);
    share->utilizedBy++;
    spinlockCntWriteRelease(&share->WLOCK_FILES);
    target->infoFiles = share;
  }

  if (!(cloneFlags & CLONE_SIGHAND))
    target->infoSignals = taskInfoSignalClone(currentTask->infoSignals);
  else {
    TaskInfoSignal *share = currentTask->infoSignals;
    spinlockAcquire(&share->LOCK_SIGNAL);
    share->utilizedBy++;
    spinlockRelease(&share->LOCK_SIGNAL);
    target->infoSignals = share;
  }

  // they get inherited, but can still be changed thread-wise!
  target->sigBlockList = currentTask->sigBlockList;

  // returns zero yk
  target->registers.rax = 0;

  // etc (https://www.felixcloutier.com/x86/syscall)
  target->registers.rip = cpu->rcx;
  target->registers.cs = GDT_USER_CODE | DPL_USER;
  target->registers.ds = GDT_USER_DATA | DPL_USER;
  target->registers.rflags = cpu->r11;
  target->registers.usermode_rsp = rsp;
  target->registers.usermode_ss = GDT_USER_DATA | DPL_USER;

  // since the scheduler, our fpu state might've changed
  asm volatile(" fxsave %0 " ::"m"(currentTask->fpuenv));
  asm("stmxcsr (%%rax)" : : "a"(&currentTask->mxcsr));

  // yk
  target->parent = currentTask;
  target->pgid = currentTask->pgid;

  // fpu stuff
  memcpy(target->fpuenv, currentTask->fpuenv, 512);
  target->mxcsr = currentTask->mxcsr;

  if (spinup)
    taskCreateFinish(target);

  return target;
}

void taskBlock(Blocking *blocking, Task *task, Spinlock *releaseAfter,
               bool apply) {
  if (!apply)
    assert(!releaseAfter);
  spinlockAcquire(&blocking->LOCK_LL_BLOCKED);

  // it's rare enough more than one task is blocked together, go through it
  BlockedTask *browse = blocking->firstBlockedTask;
  while (browse) {
    if (browse->task == task)
      debugf("[task::blocking] WARNING! Duplicate task!\n");
    browse = browse->next;
  }

  BlockedTask *blockedTask = LinkedListAllocate(
      (void **)(&blocking->firstBlockedTask), sizeof(BlockedTask));
  blockedTask->task = task;
  spinlockRelease(&blocking->LOCK_LL_BLOCKED);

  // finally block
  if (apply) {
    if (releaseAfter)
      taskSpinlockExit(task, releaseAfter);
    task->state = TASK_STATE_BLOCKED;
  }
}

void taskUnblock(Blocking *blocking) {
  spinlockAcquire(&blocking->LOCK_LL_BLOCKED);
  BlockedTask *browse = blocking->firstBlockedTask;
  while (browse) {
    BlockedTask *next = browse->next;
    if (browse->task) {
      Task *task = browse->task;
      if (task->state != TASK_STATE_DEAD)
        task->state = TASK_STATE_READY;
    }
    LinkedListRemove((void **)(&blocking->firstBlockedTask), browse);
    browse = next;
  }
  spinlockRelease(&blocking->LOCK_LL_BLOCKED);
}

// Will release lock when task isn't running via the kernel helper
void taskSpinlockExit(Task *task, Spinlock *lock) {
  assert(!task->spinlockQueueEntry);
  task->spinlockQueueEntry = lock;
}

void kernelDummyEntry() {
  while (true)
    dummyTask->state = TASK_STATE_DUMMY;
}

void initiateTasks() {
  firstTask = (Task *)malloc(sizeof(Task));
  memset(firstTask, 0, sizeof(Task));

  currentTask = firstTask;
  currentTask->id = KERNEL_TASK_ID;
  currentTask->state = TASK_STATE_READY;
  currentTask->infoPd = taskInfoPdAllocate(false);
  currentTask->infoPd->pagedir = GetPageDirectory();
  currentTask->kernel_task = true;
  currentTask->infoFs = taskInfoFsAllocate();
  currentTask->infoFiles = taskInfoFilesAllocate();
  currentTask->infoFiles->fdBitmap[0] = (uint8_t)-1;
  currentTask->infoSignals = 0; // no, just no!

  void  *tssRsp = VirtualAllocate(USER_STACK_PAGES);
  size_t tssRspSize = USER_STACK_PAGES * BLOCK_SIZE;
  memset(tssRsp, 0, tssRspSize);
  currentTask->whileTssRsp = (uint64_t)tssRsp + tssRspSize;
  taskAttachDefTermios(currentTask);

  debugf("[tasks] Current execution ready for multitasking\n");
  tasksInitiated = true;

  // task 0 represents the execution we're in right now

  // create a dummy task in case the scheduler has nothing to do
  dummyTask = taskCreateKernel((uint64_t)kernelDummyEntry, 0);
  dummyTask->state = TASK_STATE_DUMMY;
}