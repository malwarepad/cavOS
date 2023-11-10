#include "../include/syscalls.h"
#include "../include/isr.h"
#include "../include/system.h"
#include "../include/task.h"

void registerSyscall(uint32_t id, void *handler) {
  if (id > MAX_SYSCALLS)
    return;

  syscalls[id] = handler;
}

void syscallHandler(AsmPassedInterrupt *regs) {
  uint32_t id = regs->eax;
  void    *handler = syscalls[id];

  if (!handler) {
    debugf("Tried to access syscall %d (doesn't exist)!\n", id);
    return;
  }

  int ret;
  asm volatile("push %1 \n"
               "push %2 \n"
               "push %3 \n"
               "push %4 \n"
               "push %5 \n"
               "call *%6 \n"
               "pop %%ebx \n"
               "pop %%ebx \n"
               "pop %%ebx \n"
               "pop %%ebx \n"
               "pop %%ebx \n"
               : "=a"(ret)
               : "r"(regs->edi), "r"(regs->esi), "r"(regs->edx), "r"(regs->ecx),
                 "r"(regs->ebx), "r"(handler));
  regs->eax = ret;
}

bool running = false;

// System calls themselves
#define SYSCALL_TEST 0x0
static void syscallTest(char *msg) {
  debugf("Got test syscall from process %d: %s\n", current_task->id, msg);
  printf("Got test syscall from process %d: %s\n", current_task->id, msg);
}

#define SYSCALL_EXIT_TASK 0x1
static void syscallExitTask(int return_code) {
  // debugf("Exiting task %d with return code %d!\n", current_task->id,
  //        return_code);
  kill_task(current_task->id);
}

#define SYSCALL_GETPID 0x2
static uint32_t syscallGetPid() { return current_task->id; }

#define SYSCALL_GETARGC 0x3
static int syscallGetArgc() { return 6; }

static *sampleArgv[] = {"./main.c", "one", "two", "three", "four", "five"};

#define SYSCALL_GETARGV 0x4
static char *syscallGetArgv(int curr) { return sampleArgv[curr]; }

#define SYSCALL_GET_HEAP_START 0x5
static uint32_t syscallGetHeapStart() { return current_task->heap_start; }

#define SYSCALL_GET_HEAP_END 0x6
static uint32_t syscallGetHeapEnd() { return current_task->heap_end; }

#define SYSCALL_ADJUST_HEAP_END 0x7
static void syscallAdjustHeapEnd(uint32_t heap_end) {
  adjust_user_heap(current_task, heap_end);
}

#define SYSCALL_PRINT_CHAR 0x8
static void syscallPrintChar(char character) {
  serial_send(COM1, character);
  printfch(character);
}

#define SYSCALL_READ_CHAR 0x9
// todo (also fix ps/2 kb driver)
static char syscallReadChar() { return 'a'; }

void initiateSyscalls() {
  registerSyscall(SYSCALL_TEST, syscallTest);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_GETARGC, syscallGetArgc);
  registerSyscall(SYSCALL_GETARGV, syscallGetArgv);
  registerSyscall(SYSCALL_GET_HEAP_START, syscallGetHeapStart);
  registerSyscall(SYSCALL_GET_HEAP_END, syscallGetHeapEnd);
  registerSyscall(SYSCALL_ADJUST_HEAP_END, syscallAdjustHeapEnd);
  registerSyscall(SYSCALL_PRINT_CHAR, syscallPrintChar);
  registerSyscall(SYSCALL_READ_CHAR, syscallReadChar);
}
