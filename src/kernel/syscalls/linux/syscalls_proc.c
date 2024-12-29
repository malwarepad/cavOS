#include <elf.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

// process lifetime system calls (send help)

#define SYSCALL_PIPE 22
static int syscallPipe(int *fds) { return pipeOpen(fds); }

#define SYSCALL_PIPE2 293
static int syscallPipe2(int *fds, int flags) {
  if (flags && flags != O_CLOEXEC) {
    dbgSysStubf("todo flags");
    return -ENOSYS;
  }

  int out = pipeOpen(fds);
  if (out < 0)
    goto cleanup;

  if (flags) {
    // since basically the only one we support atm is the close-on-exec flag xd
    OpenFile *fd0 = fsUserGetNode(currentTask, fds[0]);
    OpenFile *fd1 = fsUserGetNode(currentTask, fds[1]);

    if (!fd0 || !fd1) {
      debugf("[syscalls::pipe2] Bad sync!\n");
      panic();
    }

    fd0->closeOnExec = true;
    fd1->closeOnExec = true;
  }

cleanup:
  return out;
}

#define SYSCALL_FORK 57
static int syscallFork() {
  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp, true, true)
      ->id;
}

#define SYSCALL_VFORK 58
static int syscallVfork() {
  Task *newTask =
      taskFork(currentTask->syscallRegs, currentTask->syscallRsp, false, false);
  int id = newTask->id;

  // no race condition today :")
  taskCreateFinish(newTask);

  currentTask->state = TASK_STATE_WAITING_VFORK;
  handControl();

  return id;
}

typedef struct CopyPtrStyle {
  int      count;
  char   **ptrPlace;
  uint8_t *valPlace;
} CopyPtrStyle;

CopyPtrStyle copyPtrStyle(char **ptr) {
  int    count = 0;
  size_t totalLen = 0;
  while (ptr[count]) {
    totalLen += strlength(ptr[count]) + 1;
    count++;
  }
  char   **ptrPlace = malloc(count * sizeof(void *));
  uint8_t *valPlace = malloc(totalLen);
  size_t   curr = 0;
  for (int i = 0; i < count; i++) {
    ptrPlace[i] = (void *)((size_t)valPlace + curr);
    curr += strlength(ptr[i]) + 1;
    memcpy(ptrPlace[i], ptr[i], strlength(ptr[i]) + 1);
  }
  CopyPtrStyle ret = {
      .count = count, .ptrPlace = ptrPlace, .valPlace = valPlace};
  return ret;
}

#define SYSCALL_EXECVE 59
static int syscallExecve(char *filename, char **argv, char **envp) {
  CopyPtrStyle arguments = copyPtrStyle(argv);
  CopyPtrStyle environment = copyPtrStyle(envp);

  char *filenameSanitized = fsSanitize(currentTask->cwd, filename);
  Task *ret = elfExecute(filenameSanitized, arguments.count, arguments.ptrPlace,
                         environment.count, environment.ptrPlace, 0);
  free(filenameSanitized);
  free(arguments.ptrPlace);
  free(arguments.valPlace);
  free(environment.ptrPlace);
  free(environment.valPlace);
  if (!ret)
    return -ENOENT;

  int targetId = currentTask->id;
  currentTask->id = taskGenerateId();

  ret->id = targetId;
  ret->parent = currentTask->parent;
  size_t cwdLen = strlength(currentTask->cwd) + 1;
  ret->cwd = malloc(cwdLen);
  memcpy(ret->cwd, currentTask->cwd, cwdLen);
  ret->umask = currentTask->umask;

  taskFilesEmpty(ret);
  taskFilesCopy(currentTask, ret, true);

  taskCreateFinish(ret);

  currentTask->noInformParent = true;
  taskKill(currentTask->id, 0);
  return 0; // will never be reached, it **replaces**
}

#define SYSCALL_EXIT_TASK 60
static void syscallExitTask(int return_code) {
  // if (return_code)
  //   panic();
  taskKill(currentTask->id, return_code);
}

#define SYSCALL_WAIT4 61
static int syscallWait4(int pid, int *wstatus, int options, struct rusage *ru) {
  if (options || ru)
    dbgSysStubf("todo options & rusage");
  /*dbgSysExtraf("WNOHANG{%d} WUNTRACED{%d} "
               "WSTOPPED{%d} WEXITED{%d} WCONTINUED{%d} "
               "WNOWAIT{%d}",
               options & WNOHANG, options & WUNTRACED, options & WSTOPPED,
               options & WEXITED, options & WCONTINUED, options & WNOWAIT);*/

  asm volatile("sti");

  // if nothing is on the side, then ensure we have something to wait() for
  if (!currentTask->childrenTerminatedAmnt) {
    int amnt = 0;

    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *browse = firstTask;
    while (browse) {
      if (browse->state == TASK_STATE_READY && browse->parent == currentTask)
        amnt++;
      browse = browse->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);

    if (!amnt)
      return -ECHILD;
  }

  // target is the item we "found"
  KilledInfo *target = 0;

  // check if specific pid item is already there
  if (pid != -1) {
    spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
    KilledInfo *browse = currentTask->firstChildTerminated;
    while (browse) {
      if (browse->pid == pid)
        break;
      browse = browse->next;
    }
    target = browse;
    spinlockRelease(&currentTask->LOCK_CHILD_TERM);

    // not there? wait for it!
    if (!target) {
      // "poll"
      currentTask->waitingForPid = pid;
      currentTask->state = TASK_STATE_WAITING_CHILD_SPECIFIC;
      handControl();

      // we're back
      currentTask->waitingForPid = 0; // just for good measure
      spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
      KilledInfo *browse = currentTask->firstChildTerminated;
      while (browse) {
        if (browse->pid == pid)
          break;
        browse = browse->next;
      }
      target = browse;
      spinlockRelease(&currentTask->LOCK_CHILD_TERM);
    }
  } else {
    // we got children, wait for any changes
    // OR just continue :")
    if (!currentTask->childrenTerminatedAmnt) {
      currentTask->state = TASK_STATE_WAITING_CHILD;
      handControl();
    }
    target = currentTask->firstChildTerminated;
  }

  spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
  if (!target) {
    debugf("[syscalls::wait4] FATAL Just fatal!");
    panic();
  }

  int output = target->pid;
  int ret = target->ret;

  // cleanup
  LinkedListRemove((void **)(&currentTask->firstChildTerminated), target);
  currentTask->childrenTerminatedAmnt--;
  spinlockRelease(&currentTask->LOCK_CHILD_TERM);

  if (wstatus)
    *wstatus = (ret & 0xff) << 8;

#if DEBUG_SYSCALLS_EXTRA
  debugf("\n%d [RET] [syscall::wait4] pid{%d} ret{%d}", currentTask->id, output,
         ret);
#endif
  return output;
}

#define SYSCALL_EXIT_GROUP 231
static void syscallExitGroup(int return_code) { syscallExitTask(return_code); }

void syscallsRegProc() {
  registerSyscall(SYSCALL_PIPE, syscallPipe);
  registerSyscall(SYSCALL_PIPE2, syscallPipe2);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_FORK, syscallFork);
  registerSyscall(SYSCALL_VFORK, syscallVfork);
  registerSyscall(SYSCALL_WAIT4, syscallWait4);
  registerSyscall(SYSCALL_EXECVE, syscallExecve);
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
}
