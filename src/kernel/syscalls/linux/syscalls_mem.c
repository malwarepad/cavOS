#include <syscalls.h>
#include <task.h>
#include <util.h>

#define SYSCALL_MMAP 9
static uint64_t syscallMmap(size_t addr, size_t length, int prot, int flags,
                            int fd, size_t pgoffset) {
  /* No point in DEBUG_SYSCALLS_ARGS'ing here */

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

#if DEBUG_SYSCALLS_STUB
  debugf(
      "[syscalls::mmap] UNIMPLEMENTED! addr{%lx} len{%lx} prot{%d} flags{%x} "
      "fd{%d} "
      "pgoffset{%x}\n",
      addr, length, prot, flags, fd, pgoffset);
#endif

  return -1;
}

#define SYSCALL_BRK 12
static uint64_t syscallBrk(uint64_t brk) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::brk] brk{%lx}\n", brk);
#endif
  if (!brk)
    return currentTask->heap_end;

  if (brk < currentTask->heap_end) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::brk] FAIL! Tried to go inside heap limits! brk{%lx} "
           "limit{%lx}\n",
           brk, currentTask->heap_end);
#endif
    return -1;
  }

  taskAdjustHeap(currentTask, brk);

  return currentTask->heap_end;
}

void syscallRegMem() {
  registerSyscall(SYSCALL_MMAP, syscallMmap);
  registerSyscall(SYSCALL_BRK, syscallBrk);
}