#include <fat32.h>
#include <gdt.h>
#include <isr.h>
#include <kb.h>
#include <schedule.h>
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

AsmPassedInterrupt *currGlobalRegs = 0;
void                syscallHandler(AsmPassedInterrupt *regs) {
  uint32_t id = regs->eax;
  void    *handler = syscalls[id];

  if (!handler) {
    regs->eax = -1;
    debugf("[syscalls] Tried to access syscall{%d} (doesn't exist)!\n", id);
    return;
  }

  currGlobalRegs = regs;

  int ret;
  // previously used "r"(regs->*)
  asm volatile("push %1 \n"
                              "push %2 \n"
                              "push %3 \n"
                              "push %4 \n"
                              "push %5 \n"
                              "push %6 \n"
                              "call *%7 \n"
                              "pop %%ebx \n"
                              "pop %%ebx \n"
                              "pop %%ebx \n"
                              "pop %%ebx \n"
                              "pop %%ebx \n"
                              "pop %%ebx \n"
               : "=a"(ret)
               : "g"(regs->ebp), "g"(regs->edi), "g"(regs->esi), "g"(regs->edx),
                 "g"(regs->ecx), "g"(regs->ebx), "g"(handler));
  regs->eax = ret;
  currGlobalRegs = regs;
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
static void syscallFork() {
  // todo: fork 🍴
  debugf("[syscalls] %d tried to fork()\n", currentTask->id);
}

#define SYSCALL_READ 0x3
static uint32_t syscallRead(int file, char *str, uint32_t count) {
  debugf("[syscalls::read] file{%d} str{%x} count{%d}\n", file, str, count);
  if (file == 0 || file == 1) {
    // console fb
    if (kbIsOccupied()) {
      return -1;
    }

    // start reading
    // todo: respect limit
    kbTaskRead(currentTask->id, str, count, true);
    schedule(); // leave this task/execution (awaiting return)

    // finalise
    uint32_t fr = currentTask->tmpRecV;
    str[fr] = '\n';
    str[fr + 1] = '\0';
    return fr + 1;
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
  uint32_t read = fsRead(browse, str, count);
  debugf("\nread = %d\n", read);
  return read;
}

#define SYSCALL_WRITE 0x4
static int syscallWrite(int file, char *str, uint32_t count) {
  debugf("[syscalls::write] file{%d} str{%x} count{%d}\n", file, str, count);
  if (file == 0 || file == 1) {
    // console fb
    for (int i = 0; i < count; i++) {
      serial_send(COM1, str[i]);
      printfch(str[i]);
    }
    return count;
  }

  return -1;
}

#define SYSCALL_OPEN 0x5
static int syscallOpen(char *filename, int flags, uint16_t mode) {
  return fsUserOpen(filename, flags, mode);
}

#define SYSCALL_CLOSE 0x6
static int syscallClose(int file) { return fsUserClose(file); }

#define SYSCALL_LSEEK 19
static int syscallLseek(uint32_t file, int offset, int whence) {
  debugf("[syscalls::seek] file{%d} offset{%d} whence{%d}\n", file, offset,
         whence);
  if (file == 0 || file == 1)
    return -1;
  return fsUserSeek(file, offset, whence);
}

#define SYSCALL_BRK 45
static int syscallBrk(uint32_t brk) {
  if (!brk)
    return currentTask->heap_end;

  if (brk < currentTask->heap_end)
    return -1;

  adjust_user_heap(currentTask, brk);

  return currentTask->heap_end;
}

#define SYSCALL_GETPID 20
static uint32_t syscallGetPid() { return currentTask->id; }

typedef struct iovec {
  void  *iov_base; /* Pointer to data.  */
  size_t iov_len;  /* Length of data.  */
} iovec;

#define SYSCALL_WRITEV 146
static int syscallWriteV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
  int cnt = 0;

  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (size_t)iov + i * sizeof(iovec);

    // debugf("[syscalls::writev] fd{%d} iov_base{%x} iov_len{%x} iovcnt{%d}\n",
    //        fd, curr->iov_base, curr->iov_len, ioVcnt);
    int singleCnt = syscallWrite(fd, curr->iov_base, curr->iov_len);
    if (singleCnt == -1)
      return cnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_MMAP2 192
static int syscallMmap2(uint32_t addr, uint32_t length, uint32_t prot,
                        uint32_t flags, uint32_t fd, uint32_t pgoffset) {
  debugf("[syscalls::mmap2] UNIMPLEMENTED! addr{%x} len{%x} prot{%x} flags{%x} "
         "fd{%x} "
         "pgoffset{%x}\n",
         addr, length, prot, flags, fd, pgoffset);

  return -1;
}

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
  registerSyscall(SYSCALL_LSEEK, syscallLseek);
  registerSyscall(SYSCALL_BRK, syscallBrk);
  registerSyscall(SYSCALL_MMAP2, syscallMmap2);
  registerSyscall(SYSCALL_WRITEV, syscallWriteV);

  debugf("[syscalls] System calls are ready to fire: %d/%d\n", countSyscalls(),
         MAX_SYSCALLS);
}
