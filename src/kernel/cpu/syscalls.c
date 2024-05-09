#include <console.h>
#include <fb.h>
#include <gdt.h>
#include <isr.h>
#include <kb.h>
#include <schedule.h>
#include <serial.h>
#include <string.h>
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
size_t syscalls[MAX_SYSCALLS] = {0};
void   registerSyscall(uint32_t id, void *handler) {
  if (id > MAX_SYSCALLS) {
    debugf("[syscalls] FATAL! Exceded limit! limit{%d} id{%d}\n", MAX_SYSCALLS,
             id);
    panic();
  }

  if (syscalls[id]) {
    debugf("[syscalls] FATAL! id{%d} found duplicate!\n", id);
    panic();
  }

  syscalls[id] = (size_t)handler;
  syscallCnt++;
}

typedef uint64_t (*SyscallHandler)(uint64_t a1, uint64_t a2, uint64_t a3,
                                   uint64_t a4, uint64_t a5, uint64_t a6);
void syscallHandler(AsmPassedInterrupt *regs) {
  uint64_t *rspPtr = (uint64_t *)((size_t)regs + sizeof(AsmPassedInterrupt));
  uint64_t  rsp = *rspPtr;
  currentTask->systemCallInProgress = true;
  currentTask->syscallRegs = regs;
  currentTask->syscallRsp = rsp;
  asm volatile("sti"); // do other task stuff while we're here!
  uint64_t id = regs->rax;
#if DEBUG_SYSCALLS
  debugf("[syscalls] id{%d}\n", id);
#endif
  size_t handler = syscalls[id];

  if (!handler) {
    regs->rax = -1;
    debugf("[syscalls] Tried to access syscall{%d} (doesn't exist)!\n", id);
    currentTask->syscallRsp = 0;
    currentTask->syscallRegs = 0;
    currentTask->systemCallInProgress = false;
    return;
  }

  long int ret = ((SyscallHandler)(handler))(regs->rdi, regs->rsi, regs->rdx,
                                             regs->r10, regs->r8, regs->r9);
#if DEBUG_SYSCALLS
  debugf("[syscalls] return_code{%d}\n", ret);
#endif

  regs->rax = ret;
  currentTask->syscallRsp = 0;
  currentTask->syscallRegs = 0;
  currentTask->systemCallInProgress = false;
}

int readHandler(OpenFile *fd, uint8_t *in, size_t limit) {
  // console fb
  // while (kbIsOccupied()) {
  // } done in kbTaskRead()

  // start reading
  kbTaskRead(currentTask->id, (char *)in, limit, true);
  asm volatile("sti"); // leave this task/execution (awaiting return)
  while (currentTask->state == TASK_STATE_WAITING_INPUT) {
  }
  printf("\n"); // you technically pressed enter, didn't you?

  // finalise
  uint32_t fr = currentTask->tmpRecV;
  if (fr < limit)
    in[fr++] = '\n';
  // only add newline if we can!

  return fr;
}

int writeHandler(OpenFile *fd, uint8_t *out, size_t limit) {
  // console fb
  for (int i = 0; i < limit; i++) {
#if DEBUG_SYSCALLS
    serial_send(COM1, out[i]);
#endif
    printfch(out[i]);
  }
  return limit;
}

int ioctlHandler(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  case 0x5413: {
    winsize *win = (winsize *)arg;
    win->ws_row = framebufferHeight / TTY_CHARACTER_HEIGHT;
    win->ws_col = framebufferWidth / TTY_CHARACTER_WIDTH;

    win->ws_xpixel = framebufferWidth;
    win->ws_ypixel = framebufferHeight;
    return 0;
    break;
  }
  default:
    return -1;
    break;
  }
}

// System calls themselves
#define SYSCALL_READ 0
static int syscallRead(int file, char *str, uint32_t count) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::read] file{%d} str{%x} count{%d}\n", file, str, count);
#endif

  OpenFile *browse = fsUserGetNode(file);
  if (!browse) {
    // handle
    return 0;
  }
  uint32_t read = fsRead(browse, (uint8_t *)str, count);
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
  OpenFile *browse = fsUserGetNode(file);
  if (!browse)
    return -1;

  uint32_t writtenBytes = fsWrite(browse, (uint8_t *)str, count);
  return writtenBytes;
}

#define SYSCALL_OPEN 2
static int syscallOpen(char *filename, int flags, uint16_t mode) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::open] filename{%s} flags{%d} mode{%d}\n", filename, flags,
         mode);
#endif
  if (!filename)
    return -1;
  return fsUserOpen(filename, flags, mode);
}

#define SYSCALL_CLOSE 3
static int syscallClose(int file) { return fsUserClose(file); }

#define SYSCALL_STAT 4
static int syscallStat(char *filename, stat *statbuf) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::stat] filename{%s} buff{%lx}\n", filename, statbuf);
#endif
  bool ret = fsStat(filename, statbuf, 0);
  if (ret)
    return 0;

  return -1;
}

#define SYSCALL_FSTAT 5
static int syscallFstat(int fd, stat *statbuf) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::fstat] fd{%d} buff{%lx}\n", fd, statbuf);
#endif
  OpenFile *file = fsUserGetNode(fd);
  if (!file)
    return -1;
  bool ret = fsStat(file->safeFilename, statbuf, 0);
  if (ret)
    return 0;

  return -1;
}

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
  if (fd == -1) { // before: !addr &&
    size_t curr = currentTask->heap_end;
#if DEBUG_SYSCALLS
    debugf("[syscalls::mmap] No placement preference, no file descriptor: "
           "addr{%lx} length{%lx}\n",
           curr, length);
#endif
    taskAdjustHeap(currentTask, currentTask->heap_end + length);
    memset((void *)curr, 0, length);
#if DEBUG_SYSCALLS
    debugf("[syscalls::mmap] Found addr{%lx}\n", curr);
#endif
    return curr;
  }
  debugf(
      "[syscalls::mmap] UNIMPLEMENTED! addr{%lx} len{%lx} prot{%d} flags{%x} "
      "fd{%d} "
      "pgoffset{%x}\n",
      addr, length, prot, flags, fd, pgoffset);

  return -1;
}

#define SYSCALL_BRK 12
static uint64_t syscallBrk(uint64_t brk) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::brk] brk{%lx}\n", brk);
#endif
  if (!brk)
    return currentTask->heap_end;

  if (brk < currentTask->heap_end)
    return -1;

  taskAdjustHeap(currentTask, brk);

  return currentTask->heap_end;
}

#define SYSCALL_RT_SIGACTION 13
static int syscallRtSigaction(int sig, const struct sigaction *act,
                              struct sigaction *oact, size_t sigsetsize) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::sigaction] sig{%d} act{%lx} oact{%lx} sigsetsize{%lx}\n",
         sig, act, oact, sigsetsize);
#endif
  debugf("[syscalls::sigaction] UNIMPLEMENTED!\n");
  return -1;
}

#define SYSCALL_RT_SIGPROCMASK 14
static int syscallRtSigprocmask(int how, sigset_t *nset, sigset_t *oset,
                                size_t sigsetsize) {
#if DEBUG_SYSCALLS
  debugf(
      "[syscalls::sigprocmask] how{%d} nset{%lx} oset{%lx} sigsetsize{%lx}\n",
      how, nset, oset, sigsetsize);
#endif
  debugf("todo!\n");
  return -1;
}

#define SYSCALL_IOCTL 16
static int syscallIoctl(int fd, unsigned long request, void *arg) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::ioctl] fd{%d} req{%lx} arg{%lx}\n", fd, request, arg);
#endif
  OpenFile *browse = fsUserGetNode(fd);
  if (!browse || browse->mountPoint != MOUNT_POINT_SPECIAL)
    return -1;

  SpecialFile *special = (SpecialFile *)browse->dir;
  if (!special)
    return -1;

  int ret = special->ioctlHandler(browse, request, arg);
  if (ret < 0)
    debugf("[syscalls::ioctl] UNIMPLEMENTED! fd{%d} req{%lx} arg{%lx}\n", fd,
           request, arg);
  return ret;
}

#define SYSCALL_WRITEV 20
static int syscallWriteV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
  int cnt = 0;

  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (iovec *)((size_t)iov + i * sizeof(iovec));

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

#define SYSCALL_DUP2 33
static int syscallDup2(uint32_t oldFd, uint32_t newFd) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::dup2] old{%d} new{%d}\n", oldFd, newFd);
#endif
  // todo: treat 0, 1 and the like FDs like actual files
  debugf("[syscalls::dup2] UNIMPLEMENTED! old{%d} new{%d}\n", oldFd, newFd);
  return -1;
}

#define SYSCALL_GETPID 39
static uint32_t syscallGetPid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getpid] pid{%d}\n", currentTask->id);
#endif
  return currentTask->id;
}

#define SYSCALL_FORK 57
static int syscallFork() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::fork] parent{%d}\n", currentTask->id);
#endif
  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp);
}

#define SYSCALL_EXIT_TASK 60
static void syscallExitTask(int return_code) {
#if DEBUG_SYSCALLS
  debugf("[scheduler] Exiting task{%d} with return code{%d}!\n",
         currentTask->id, return_code);
#endif
  // if (return_code)
  //   panic();
  taskKill(currentTask->id);

  // should not return
}

#define SYSCALL_GETCWD 79
static int syscallGetcwd(char *buff, size_t size) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getcwd] buff{%lx} size{%lx}\n", buff, size);
#endif
  size_t realLength = strlength(currentTask->cwd) + 1;
  if (size < realLength)
    return -1;
  memcpy(buff, currentTask->cwd, realLength);

  return realLength;
}

#define SYSCALL_CHDIR 80
static int syscallChdir(char *newdir) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::chdir] place{%s}\n", newdir);
#endif
  bool ret = taskChangeCwd(newdir);
  if (ret)
    return 0;

  return -1;
}

#define SYSCALL_GETUID 102
static int syscallGetuid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getuid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETGID 104
static int syscallGetgid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getgid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETEUID 107
static int syscallGeteuid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::geteuid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETEGID 108
static int syscallGetegid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getegid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_SETPGID 109
static int syscallSetpgid(int pid, int pgid) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::setpgid] pid{%d} pgid{%d}\n", pid, pgid);
#endif

  if (!pid)
    pid = currentTask->id;

  Task *task = taskGet(pid);
  if (!task)
    return -1;

  task->pgid = pgid;
  return 0;
}

#define SYSCALL_GETPGID 121
static int syscallGetpgid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::getpgid]\n");
#endif
  return currentTask->pgid;
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
  }

  return -1;
}

#define SYSCALL_GET_TID 186
static int syscallGetTid() {
#if DEBUG_SYSCALLS
  debugf("[syscalls::gettid]\n");
#endif
  return currentTask->id;
}

#define SYSCALL_SET_TID_ADDR 218
static int syscallSetTidAddr(int *tidptr) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::settid] tidptr{%lx}\n", tidptr);
#endif
  return currentTask->id;
}

#define SYSCALL_EXIT_GROUP 231
static void syscallExitGroup(int return_code) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::exitgroup]\n");
#endif
  syscallExitTask(return_code);
}

#define SYSCALL_OPENAT 257
static int syscallOpenAt(int fd, char *filename, int flags, uint16_t mode) {
#if DEBUG_SYSCALLS
  debugf("[syscalls::openat] fd{%d} filename{%s} flags{%d} mode{%d}\n", fd,
         filename, flags, mode);
#endif
  // todo: yeah, fix this
  return fsUserOpen(filename, flags, mode);
}

#define SYSCALL_TEST 405
static int syscallTest(char *msg) {
  debugf("[syscalls] Got test syscall from process{%d}: %s\n", currentTask->id,
         msg);
  return strlength(msg);
  // printf("Got test syscall from process %d: %s\n", currentTask->id, msg);
}

void initiateSyscalls() {
  registerSyscall(SYSCALL_TEST, syscallTest);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_WRITE, syscallWrite);
  registerSyscall(SYSCALL_READ, syscallRead);
  registerSyscall(SYSCALL_OPEN, syscallOpen);
  registerSyscall(SYSCALL_OPENAT, syscallOpenAt);
  registerSyscall(SYSCALL_CLOSE, syscallClose);
  registerSyscall(SYSCALL_LSEEK, syscallLseek);
  registerSyscall(SYSCALL_BRK, syscallBrk);
  registerSyscall(SYSCALL_MMAP, syscallMmap);
  registerSyscall(SYSCALL_WRITEV, syscallWriteV);
  registerSyscall(SYSCALL_PRCTL, syscallPrctl);
  registerSyscall(SYSCALL_SET_TID_ADDR, syscallSetTidAddr);
  registerSyscall(SYSCALL_GET_TID, syscallGetTid);
  registerSyscall(SYSCALL_GETCWD, syscallGetcwd);
  registerSyscall(SYSCALL_IOCTL, syscallIoctl);
  registerSyscall(SYSCALL_FORK, syscallFork);
  registerSyscall(SYSCALL_RT_SIGACTION, syscallRtSigaction);
  registerSyscall(SYSCALL_RT_SIGPROCMASK, syscallRtSigprocmask);
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
  registerSyscall(SYSCALL_STAT, syscallStat);
  registerSyscall(SYSCALL_FSTAT, syscallFstat);
  registerSyscall(SYSCALL_GETUID, syscallGetuid);
  registerSyscall(SYSCALL_DUP2, syscallDup2);
  registerSyscall(SYSCALL_GETEUID, syscallGeteuid);
  registerSyscall(SYSCALL_GETGID, syscallGetgid);
  registerSyscall(SYSCALL_GETEGID, syscallGetegid);
  registerSyscall(SYSCALL_GETPGID, syscallGetpgid);
  registerSyscall(SYSCALL_SETPGID, syscallSetpgid);
  registerSyscall(SYSCALL_CHDIR, syscallChdir);

  debugf("[syscalls] System calls are ready to fire: %d/%d\n", syscallCnt,
         MAX_SYSCALLS);
}
