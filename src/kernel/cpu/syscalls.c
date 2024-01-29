#include <fat32.h>
#include <isr.h>
#include <serial.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

void registerSyscall(uint32_t id, void *handler) {
  if (id > MAX_SYSCALLS)
    return;

  syscalls[id] = handler;
}

void syscallHandler(AsmPassedInterrupt *regs) {
  uint32_t id = regs->eax;
  void    *handler = syscalls[id];

  if (!handler) {
    debugf("[syscalls] Tried to access syscall{%d} (doesn't exist)!\n", id);
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
  debugf("[syscalls] Got test syscall from process{%d}: %s\n", currentTask->id,
         msg);
  printf("Got test syscall from process %d: %s\n", currentTask->id, msg);
}

#define SYSCALL_EXIT_TASK 0x1
static void syscallExitTask(int return_code) {
  debugf("[scheduler] Exiting task{%d} with return code{%d}!\n",
         currentTask->id, return_code);
  kill_task(currentTask->id);
}

#define SYSCALL_FORK 2
static void syscallFork(int file, char *str, uint32_t count) {
  // todo: fork 🍴
  debugf("[syscalls] %d tried to fork()\n", currentTask->id);
}

#define SYSCALL_READ 0x3
static uint32_t syscallRead(int file, char *str, uint32_t count) {
  debugf("[syscalls::read] file{%d} str{%x} count{%d}\n", file, str, count);
  if (file == 0 || file == 1) {
    // console fb
    // todo: respect limit, allow multitasking, etc
    readStr(str);
    return count;
  }

  OpenFile *browse = currentTask->firstFile;
  while (browse) {
    if (browse->id == file)
      break;
    browse = browse->next;
  }
  if (!browse) {
    // handle
    return 0;
  }
  // uint32_t read = readForTask(browse, str, count);
  // readFileContents(&str, browse->dir);
  return count;
}

#define SYSCALL_WRITE 0x4
static void syscallWrite(int file, char *str, uint32_t count) {
  debugf("[syscalls::write] file{%d} str{%x} count{%d}\n", file, str, count);
  if (file == 0 || file == 1) {
    // console fb
    for (int i = 0; i < count; i++) {
      serial_send(COM1, str[i]);
      printfch(str[i]);
    }
    return;
  }
}

#define SYSCALL_OPEN 0x5
static int syscallOpen(char *filename, int flags, uint16_t mode) {
  return fsUserOpen(filename, flags, mode);
}

#define SYSCALL_CLOSE 0x6
static int syscallClose(int file) { return fsUserClose(file); }

#define SYSCALL_GETPID 20
static uint32_t syscallGetPid() { return currentTask->id; }

#define SYSCALL_GETARGC 400
static int syscallGetArgc() { return 6; }

static *sampleArgv[] = {"./main.c", "one", "two", "three", "four", "five"};

#define SYSCALL_GETARGV 401
static char *syscallGetArgv(int curr) { return sampleArgv[curr]; }

#define SYSCALL_GET_HEAP_START 402
static uint32_t syscallGetHeapStart() { return currentTask->heap_start; }

#define SYSCALL_GET_HEAP_END 403
static uint32_t syscallGetHeapEnd() { return currentTask->heap_end; }

#define SYSCALL_ADJUST_HEAP_END 404
static void syscallAdjustHeapEnd(uint32_t heap_end) {
  adjust_user_heap(currentTask, heap_end);
}

uint16_t countSyscalls() {
  uint16_t tmp = 0;
  for (int i = 0; i < MAX_SYSCALLS; i++) {
    if (syscalls[i])
      tmp++;
    else
      return tmp;
  }

  return tmp;
}

void initiateSyscalls() {
  registerSyscall(SYSCALL_TEST, syscallTest);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_GETARGC, syscallGetArgc);
  registerSyscall(SYSCALL_GETARGV, syscallGetArgv);
  registerSyscall(SYSCALL_GET_HEAP_START, syscallGetHeapStart);
  registerSyscall(SYSCALL_GET_HEAP_END, syscallGetHeapEnd);
  registerSyscall(SYSCALL_ADJUST_HEAP_END, syscallAdjustHeapEnd);
  registerSyscall(SYSCALL_WRITE, syscallWrite);
  registerSyscall(SYSCALL_READ, syscallRead);
  registerSyscall(SYSCALL_OPEN, syscallOpen);
  registerSyscall(SYSCALL_CLOSE, syscallClose);

  debugf("[syscalls] System calls are ready to fire: %d/%d\n", countSyscalls(),
         MAX_SYSCALLS);
}
