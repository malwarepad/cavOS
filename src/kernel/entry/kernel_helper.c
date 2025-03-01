#include <kernel_helper.h>
#include <nic_controller.h>
#include <paging.h>
#include <system.h>
#include <task.h>
#include <types.h>
#include <util.h>
#include <vmm.h>

// Kernel helper thread entry...
// It will help with cleaning up processes, managing networking, etc while
// adhering to spinlocks (in contrast with interrupts)
// Copyright (C) 2024 Panagiotis

Task *netHelperTask = 0;

void helperNet() {
  while (true) {
    if (netQueueRead == netQueueWrite) {
      // empty :p
      return;
    }

    handlePacket(netQueue[netQueueRead].nic, netQueue[netQueueRead].buff,
                 netQueue[netQueueRead].packetLength);
    netQueueRead = (netQueueRead + 1) % QUEUE_MAX;
  }
}

void helperReaper() {
  spinlockAcquire(&LOCK_REAPER);
  if (reaperTask) {
    if (reaperTask->state != TASK_STATE_DEAD)
      goto end;

    // we have a task to kill!
    taskFreeChildren(reaperTask); // free the children

    // free stacks
    size_t stackSize = USER_STACK_PAGES * BLOCK_SIZE;
    void  *tssRsp = (void *)(reaperTask->whileTssRsp - stackSize);
    void  *syscallRsp = (void *)(reaperTask->whileSyscallRsp - stackSize);
    VirtualFree(tssRsp, USER_STACK_PAGES);
    VirtualFree(syscallRsp, USER_STACK_PAGES);

    // todo: free() the task in a safe manner. also implement some system for
    // editing the global LL without race conditions

    // done! continue listening
    reaperTask = 0;
  }

end:
  spinlockRelease(&LOCK_REAPER);
}

void kernelHelpEntry() {
  while (true) {
    helperNet();
    helperReaper();

    handControl();
  }
}

void initiateKernelThreads() {
  netHelperTask = taskCreateKernel((size_t)kernelHelpEntry, 0);
}
