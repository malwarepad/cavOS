#include <console.h>
#include <gdt.h>
#include <isr.h>
#include <kb.h>
#include <schedule.h>
#include <serial.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

#define DEBUG_SYSCALLS 1

bool checkSyscallInst() {
  uint32_t eax = 0x80000001, ebx = 0, ecx = 0, edx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  return (edx >> 11) & 1;
}

extern void syscall_entry();
void        initiateSyscallInst() {
  if (!checkSyscallInst()) {
    debugf("[syscalls] FATAL! No support for the syscall instruction found!\n");
    panic();
  }

  uint64_t star_reg = rdmsr(MSRID_STAR);
  star_reg &= 0x00000000ffffffff;

  star_reg |= ((uint64_t)GDT_USER_CODE - 16) << 48;
  star_reg |= ((uint64_t)GDT_KERNEL_CODE) << 32;
  wrmsr(MSRID_STAR, star_reg);

  // entry defined in isr.asm
  wrmsr(MSRID_LSTAR, (uint64_t)syscall_entry);

  uint64_t efer_reg = rdmsr(MSRID_EFER);
  efer_reg |= 1 << 0;
  wrmsr(MSRID_EFER, efer_reg);

  wrmsr(MSRID_FMASK, RFLAGS_IF | RFLAGS_DF);
}

uint32_t syscallCnt = 0;
#define MAX_SYSCALLS 420
uint64_t syscalls[MAX_SYSCALLS] = {0};
void     registerSyscall(uint32_t id, void *handler) {
  if (id > MAX_SYSCALLS) {
    debugf("[syscalls] FATAL! Exceded limit! limit{%d} id{%d}\n", MAX_SYSCALLS,
               id);
    panic();
  }

  if (syscalls[id]) {
    debugf("[syscalls] FATAL! id{%d} found duplicate!\n");
    panic();
  }

  syscalls[id] = handler;
  syscallCnt++;
}

typedef uint64_t (*SyscallHandler)(uint64_t a1, uint64_t a2, uint64_t a3,
                                   uint64_t a4, uint64_t a5, uint64_t a6);
void syscallHandler(AsmPassedInterrupt *regs) {
  uint64_t id = regs->rax;
  debugf("SYSCALL %d\n", id);
  void *handler = syscalls[id];

  if (!handler) {
    regs->rax = -1;
    debugf("[syscalls] Tried to access syscall{%d} (doesn't exist)!\n", id);
    return;
  }

  long int ret = ((SyscallHandler)(handler))(regs->rdi, regs->rsi, regs->rdx,
                                             regs->r10, regs->r8, regs->r9);

  regs->rax = ret;
}

bool running = false;

// System calls themselves
#define SYSCALL_READ 0
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
    // TODO! (below)
    // schedule(); // leave this task/execution (awaiting return)

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
#if DEBUG_SYSCALLS
  debugf("\nread = %d\n", read);
#endif
  return read;
}

#define SYSCALL_WRITE 1
static int syscallWrite(int file, char *str, uint32_t count) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::write] file{%d} str{%x} count{%d}\n", file, str, count);
#endif
  if (file == 0 || file == 1 || file == 2) {
    // console fb
    for (int i = 0; i < count; i++) {
      serial_send(COM1, str[i]);
      printfch(str[i]);
    }
    return count;
  }

  return -1;
}

#define SYSCALL_OPEN 2
static int syscallOpen(char *filename, int flags, uint16_t mode) {
  return fsUserOpen(filename, flags, mode);
}

#define SYSCALL_CLOSE 3
static int syscallClose(int file) { return fsUserClose(file); }

#define SYSCALL_LSEEK 8
static int syscallLseek(uint32_t file, int offset, int whence) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::seek] file{%d} offset{%d} whence{%d}\n", file, offset,
         whence);
#endif
  if (file == 0 || file == 1)
    return -1;
  return fsUserSeek(file, offset, whence);
}

#define SYSCALL_MMAP 9
static uint64_t syscallMmap(size_t addr, size_t length, int prot, int flags,
                            int fd, size_t pgoffset) {
  debugf(
      "[syscalls::mmap] UNIMPLEMENTED! addr{%lx} len{%lx} prot{%d} flags{%x} "
      "fd{%d} "
      "pgoffset{%x}\n",
      addr, length, prot, flags, fd, pgoffset);

  size_t curr = currentTask->heap_end;
  debugf("%lx\n", curr);
  adjust_user_heap(currentTask, currentTask->heap_end + length);

  return curr;
}

#define SYSCALL_BRK 12
static uint64_t syscallBrk(uint64_t brk) {
  if (!brk)
    return currentTask->heap_end;

  if (brk < currentTask->heap_end)
    return -1;

  adjust_user_heap(currentTask, brk);

  return currentTask->heap_end;
}

#define SYSCALL_WRITEV 20
static int syscallWriteV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
  int cnt = 0;

  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (size_t)iov + i * sizeof(iovec);

#if DEBUG_SYSCALLS
    debugf("[syscalls::writev] fd{%d} iov_base{%x} iov_len{%x} iovcnt{%d}\n",
           fd, curr->iov_base, curr->iov_len, ioVcnt);
#endif
    int singleCnt = syscallWrite(fd, curr->iov_base, curr->iov_len);
    if (singleCnt == -1)
      return cnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_GETPID 39
static uint32_t syscallGetPid() { return currentTask->id; }

#define SYSCALL_FORK 57
static void syscallFork() {
  // todo: fork 🍴
  debugf("[syscalls] %d tried to fork()\n", currentTask->id);
}

#define SYSCALL_EXIT_TASK 60
static void syscallExitTask(int return_code) {
#if DEBUG_SYSCALLS
  debugf("[scheduler] Exiting task{%d} with return code{%d}!\n",
         currentTask->id, return_code);
#endif
  kill_task(currentTask->id);

  // should not return
}

#define SYSCALL_PRCTL 158
static int syscallPrctl(int code, size_t addr) {
#if DEBUG_SYSCALLS
  debugf("[syscalls] Prctl: code{%d} addr{%lx}!\n", code, addr);
#endif

  switch (code) {
  case 0x1002:
    currentTask->fsbase = addr;
    wrmsr(MSRID_FSBASE, currentTask->fsbase);

    return 0;
    break;
  default:
    return -1;
    break;
  }
}

#define SYSCALL_GET_TID 186
static int syscallGetTid() { return currentTask->id; }

#define SYSCALL_SET_TID_ADDR 218
static int syscallSetTidAddr(int *tidptr) {
  debugf("[syscalls] tid: %lx\n", tidptr);
  return -1;
}

#define SYSCALL_GET_HEAP_START 402
static uint32_t syscallGetHeapStart() { return currentTask->heap_start; }

#define SYSCALL_GET_HEAP_END 403
static uint32_t syscallGetHeapEnd() { return currentTask->heap_end; }

#define SYSCALL_ADJUST_HEAP_END 404
static void syscallAdjustHeapEnd(uint32_t heap_end) {
  adjust_user_heap(currentTask, heap_end);
}

#define SYSCALL_TEST 405
static void syscallTest(char *msg) {
  debugf("[syscalls] Got test syscall from process{%d}: %s\n", currentTask->id,
         msg);
  printf("Got test syscall from process %d: %s\n", currentTask->id, msg);
}

void initiateSyscalls() {
  registerSyscall(SYSCALL_TEST, syscallTest);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_GET_HEAP_START, syscallGetHeapStart);
  registerSyscall(SYSCALL_GET_HEAP_END, syscallGetHeapEnd);
  registerSyscall(SYSCALL_ADJUST_HEAP_END, syscallAdjustHeapEnd);
  registerSyscall(SYSCALL_WRITE, syscallWrite);
  registerSyscall(SYSCALL_READ, syscallRead);
  registerSyscall(SYSCALL_OPEN, syscallOpen);
  registerSyscall(SYSCALL_CLOSE, syscallClose);
  registerSyscall(SYSCALL_LSEEK, syscallLseek);
  registerSyscall(SYSCALL_BRK, syscallBrk);
  registerSyscall(SYSCALL_MMAP, syscallMmap);
  registerSyscall(SYSCALL_WRITEV, syscallWriteV);
  registerSyscall(SYSCALL_PRCTL, syscallPrctl);
  registerSyscall(SYSCALL_SET_TID_ADDR, syscallSetTidAddr);
  registerSyscall(SYSCALL_GET_TID, syscallGetTid);

  debugf("[syscalls] System calls are ready to fire: %d/%d\n", syscallCnt,
         MAX_SYSCALLS);
}
