#include <syscalls.h>
#include <task.h>
#include <util.h>

#define SYSCALL_MMAP 9
static uint64_t syscallMmap(size_t addr, size_t length, int prot, int flags,
                            int fd, size_t pgoffset) {
  length = DivRoundUp(length, 0x1000) * 0x1000;
  /* No point in DEBUG_SYSCALLS_ARGS'ing here */

  if (!addr && fd == -1) { // before: !addr &&
    size_t curr = currentTask->mmap_end;
#if DEBUG_SYSCALLS
    debugf("[syscalls::mmap] No placement preference, no file descriptor: "
           "addr{%lx} length{%lx}\n",
           curr, length);
#endif
    taskAdjustHeap(currentTask, currentTask->mmap_end + length,
                   &currentTask->mmap_start, &currentTask->mmap_end);
    memset((void *)curr, 0, length);
#if DEBUG_SYSCALLS
    debugf("[syscalls::mmap] Found addr{%lx}\n", curr);
#endif
    return curr;
  } else if (fd != -1) {
    OpenFile *file = fsUserGetNode(fd);
    if (!file || file->mountPoint != MOUNT_POINT_SPECIAL)
      return -1;

    SpecialFile *special = (SpecialFile *)file->dir;
    if (!special)
      return -1;

    return special->handlers->mmap(addr, length, prot, flags, fd, pgoffset);
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

  taskAdjustHeap(currentTask, brk, &currentTask->heap_start,
                 &currentTask->heap_end);

  return currentTask->heap_end;
}

void syscallRegMem() {
  registerSyscall(SYSCALL_MMAP, syscallMmap);
  registerSyscall(SYSCALL_BRK, syscallBrk);
}