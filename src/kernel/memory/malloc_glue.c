#include <malloc_glue.h>
#include <util.h>
#include <vmm.h>

#define DEBUG_DLMALLOC_GLUE 0

void *sbrk(size_t increment) {
#if DEBUG_DLMALLOC_GLUE
  debugf("[dlmalloc::sbrk] size{%lx}\n", increment);
#endif
  if (!increment)
    increment = 0x1000;
  // return 0;

  uint64_t blocks = DivRoundUp(increment, BLOCK_SIZE);
  void    *virt = VirtualAllocate(blocks);
  memset(virt, 0, blocks * BLOCK_SIZE);

#if DEBUG_DLMALLOC_GLUE
  debugf("[dlmalloc::sbrk] found{%lx}\n", virt);
#endif
  return virt;
}

int  __errnoF = 0;
int  __errno_location = 0;
void __errno() { return &__errnoF; }

void abort() {
  debugf("[dlmalloc::abort] errno{%x}!\n", __errno);
  panic();
}