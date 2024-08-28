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

#define SYSCALL_FORK 57
static int syscallFork() {
  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp);
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

  Task *ret = elfExecute(filename, arguments.count, arguments.ptrPlace,
                         environment.count, environment.ptrPlace, 0);
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
#if DEBUG_SYSCALLS_STUB
  if (options || ru)
    debugf("[syscall::wait4] UNIMPLEMENTED options{%d} rusage{%lx}!\n", options,
           ru);
  debugf("[syscall::wait4] UNIMPLEMENTED WNOHANG{%d} WUNTRACED{%d} "
         "WSTOPPED{%d} WEXITED{%d} WCONTINUED{%d} "
         "WNOWAIT{%d}\n",
         options & WNOHANG, options & WUNTRACED, options & WSTOPPED,
         options & WEXITED, options & WCONTINUED, options & WNOWAIT);
#endif

  asm volatile("sti");

  if (pid == -1) {
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

    // we got children, wait for any changes
    // OR just continue :")
    while (!currentTask->childrenTerminatedAmnt)
      ;

    spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
    if (!currentTask->firstChildTerminated) {
      debugf("[syscalls::wait4] FATAL Just fatal!");
      panic();
    }

    int output = currentTask->firstChildTerminated->pid;
    int ret = currentTask->firstChildTerminated->ret;

    // cleanup
    LinkedListRemove((void **)(&currentTask->firstChildTerminated),
                     currentTask->firstChildTerminated);
    currentTask->childrenTerminatedAmnt--;
    spinlockRelease(&currentTask->LOCK_CHILD_TERM);

    if (wstatus)
      *wstatus = (ret & 0xff) << 8;

#if DEBUG_SYSCALLS_EXTRA
    debugf("[syscall::wait4] ret{%d} ret{%d}\n", output, ret);
#endif
    return output;
  } else {
#if DEBUG_SYSCALLS_STUB
    debugf("[syscall::wait4] UNIMPLEMENTED pid{%d}!\n", pid);
#endif
  }

  return -1;
}

#define SYSCALL_EXIT_GROUP 231
static void syscallExitGroup(int return_code) { syscallExitTask(return_code); }

void syscallsRegProc() {
  registerSyscall(SYSCALL_PIPE, syscallPipe);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_FORK, syscallFork);
  registerSyscall(SYSCALL_WAIT4, syscallWait4);
  registerSyscall(SYSCALL_EXECVE, syscallExecve);
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
}
