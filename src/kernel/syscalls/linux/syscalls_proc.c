#include <syscalls.h>
#include <system.h>
#include <task.h>

#define SYSCALL_FORK 57
static int syscallFork() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::fork] parent{%d}\n", currentTask->id);
#endif

  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp);
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
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
}
