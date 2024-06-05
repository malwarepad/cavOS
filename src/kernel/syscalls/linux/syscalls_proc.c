#include <elf.h>
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
  int    argc = 0;
  size_t totalLen = 0;
  while (argv[argc]) {
    totalLen += strlength(argv[argc]) + 1;
    argc++;
  }
  char   **ptrPlace = malloc(argc);
  uint8_t *valPlace = malloc(totalLen);
  size_t   curr = 0;
  for (int i = 0; i < argc; i++) {
    ptrPlace[i] = (void *)((size_t)valPlace + curr);
    curr += strlength(argv[i]) + 1;
    memcpy(ptrPlace[i], argv[i], strlength(argv[i]) + 1);
  }

  currentTask->id = taskGenerateId();

  Task *ret = elfExecute(filename, argc, ptrPlace, 0);
  free(ptrPlace);
  free(valPlace);
  if (!ret)
    return -1;

  ret->id = currentTask->id;
  ret->parent = currentTask->parent;
  taskCreateFinish(ret);

  taskKill(currentTask->id);
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
  taskKill(currentTask->id);
}

#define SYSCALL_WAIT4 61
static int syscallWait4(int pid, int *start_addr, int options,
                        struct rusage *ru) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscall::wait4] pid{%d} start_addr{%lx} options{%d} rusage{%lx}!\n",
         pid, start_addr, options, ru);
#endif
#if DEBUG_SYSCALLS_STUB
  if (options || ru)
    debugf("[syscall::wait4] UNIMPLEMENTED options{%d} rusage{%lx}!\n", options,
           ru);
#endif

  if (pid == -1) {
    int   amnt = 0;
    Task *browse = firstTask;
    while (browse) {
      if (browse->parent == currentTask)
        amnt++;
      browse = browse->next;
    }

    if (!amnt)
      return -1;
    currentTask->lastChildKilled = 0;

    while (!currentTask->lastChildKilled) {
    }

    return currentTask->lastChildKilled;
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
