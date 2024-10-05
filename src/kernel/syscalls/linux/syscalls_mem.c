#include <linux.h>
#include <paging.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>

#define SYSCALL_MMAP 9
static uint64_t syscallMmap(size_t addr, size_t length, int prot, int flags,
                            int fd, size_t pgoffset) {
  if (length == 0)
    return -EINVAL;

  length = DivRoundUp(length, 0x1000) * 0x1000;
  /* No point in DEBUG_SYSCALLS_ARGS'ing here */
  if (!addr)
    flags &= ~MAP_FIXED;

  if (flags & MAP_FIXED && flags & MAP_ANONYMOUS) {
    size_t pages = DivRoundUp(length, PAGE_SIZE);

    for (int i = 0; i < pages; i++)
      VirtualMap(addr + i * PAGE_SIZE, PhysicalAllocate(1), PF_RW | PF_USER);

    memset((void *)addr, 0, pages * PAGE_SIZE);
    return addr;
  }

  if (!addr && fd == -1 &&
      (flags & ~MAP_FIXED & ~MAP_PRIVATE) ==
          MAP_ANONYMOUS) { // before: !addr &&
    size_t curr = currentTask->mmap_end;
#if DEBUG_SYSCALLS_EXTRA
    debugf("[syscalls::mmap] No placement preference, no file descriptor: "
           "addr{%lx} length{%lx}\n",
           curr, length);
#endif
    taskAdjustHeap(currentTask, currentTask->mmap_end + length,
                   &currentTask->mmap_start, &currentTask->mmap_end);
    memset((void *)curr, 0, length);
#if DEBUG_SYSCALLS_EXTRA
    debugf("[syscalls::mmap] Found addr{%lx}\n", curr);
#endif
    return curr;
  } else if (!addr && fd == -1 &&
             (flags & ~MAP_FIXED & ~MAP_PRIVATE & ~MAP_SHARED) ==
                 MAP_ANONYMOUS &&
             (flags & ~MAP_FIXED & ~MAP_PRIVATE & ~MAP_ANONYMOUS) ==
                 MAP_SHARED) {
    debugf("[syscalls::mmap] FATAL! Shared memory is unstable asf!\n");
    panic();
    size_t base = currentTask->mmap_end;
    size_t pages = DivRoundUp(length, PAGE_SIZE);
    currentTask->mmap_end += pages * PAGE_SIZE;

    for (int i = 0; i < pages; i++)
      VirtualMap(base + i * PAGE_SIZE, PhysicalAllocate(1),
                 PF_RW | PF_USER | PF_SHARED);

    return base;
  } else if (fd != -1) {
    OpenFile *file = fsUserGetNode(currentTask, fd);
    if (!file)
      return -1;

    if (!file->handlers->mmap)
      return -1;
    return file->handlers->mmap(addr, length, prot, flags, file, pgoffset);
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

#define SYSCALL_MUNMAP 11
static int syscallMunmap(uint64_t addr, size_t len) {
  // todo
  return 0;
}

#define SYSCALL_BRK 12
static uint64_t syscallBrk(uint64_t brk) {
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
  registerSyscall(SYSCALL_MUNMAP, syscallMunmap);
  registerSyscall(SYSCALL_BRK, syscallBrk);
}