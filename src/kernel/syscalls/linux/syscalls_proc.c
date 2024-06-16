#include <elf.h>
#include <linux.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

#define SYSCALL_FORK 57
static int syscallFork() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::fork] parent{%d}\n", currentTask->id);
#endif

  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp);
}

#define SYSCALL_EXECVE 59
static int syscallExecve(char *filename, char **argv, char **envp) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::execve] Replacing task{%d} with filename{%s}!\n",
         currentTask->id, filename);
#endif
  // todo: envp!
  int    argc = 0;
  size_t totalLen = 0;
  while (argv[argc]) {
    totalLen += strlength(argv[argc]) + 1;
    argc++;
  }
  char   **ptrPlace = malloc(argc * sizeof(void *));
  uint8_t *valPlace = malloc(totalLen);
  size_t   curr = 0;
  for (int i = 0; i < argc; i++) {
    ptrPlace[i] = (void *)((size_t)valPlace + curr);
    curr += strlength(argv[i]) + 1;
    memcpy(ptrPlace[i], argv[i], strlength(argv[i]) + 1);
  }

  int targetId = currentTask->id;
  currentTask->id = taskGenerateId();

  Task *ret = elfExecute(filename, argc, ptrPlace, 0);
  free(ptrPlace);
  free(valPlace);
  if (!ret)
    return -1;

  ret->id = targetId;
  ret->parent = currentTask->parent;
  size_t cwdLen = strlength(currentTask->cwd) + 1;
  ret->cwd = malloc(cwdLen);
  memcpy(ret->cwd, currentTask->cwd, cwdLen);
  taskCreateFinish(ret);

  currentTask->noInformParent = true;
  taskKill(currentTask->id, 0);
  return 0; // will never be reached, it **replaces**
}

#define SYSCALL_EXIT_TASK 60
static void syscallExitTask(int return_code) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[scheduler] Exiting task{%d} with return code{%d}!\n",
         currentTask->id, return_code);
#endif
  // if (return_code)
  //   panic();
  taskKill(currentTask->id, return_code);
}

#define SYSCALL_WAIT4 61
static int syscallWait4(int pid, int *wstatus, int options, struct rusage *ru) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscall::wait4] pid{%d} wstatus{%lx} options{%d} rusage{%lx}!\n",
         pid, wstatus, options, ru);
#endif
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

  if (pid == -1) {
    if (!currentTask->lastChildKilled.pid) {
      int   amnt = 0;
      Task *browse = firstTask;
      while (browse) {
        if (browse->parent == currentTask)
          amnt++;
        browse = browse->next;
      }

      if (!amnt)
        return -1;
      // currentTask->lastChildKilled.pid = 0;

      while (!currentTask->lastChildKilled.pid) {
      }
    }

    int output = currentTask->lastChildKilled.pid;
    int ret = currentTask->lastChildKilled.ret;

    // clean
    currentTask->lastChildKilled.pid = 0;
    currentTask->lastChildKilled.ret = 0;

    if (wstatus)
      *wstatus = ((ret & 0xff) & 0xffff00ff) << 8;

#if DEBUG_SYSCALLS
    debugf("[syscall::wait4] ret{%d}\n", output);
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
static void syscallExitGroup(int return_code) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::exitgroup]\n");
#endif
  syscallExitTask(return_code);
}

void syscallsRegProc() {
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_FORK, syscallFork);
  registerSyscall(SYSCALL_WAIT4, syscallWait4);
  registerSyscall(SYSCALL_EXECVE, syscallExecve);
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
}
