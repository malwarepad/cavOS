#include <console.h>
#include <dev.h>
#include <kernel_helper.h>
#include <nic_controller.h>
#include <paging.h>
#include <poll.h>
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
    if (reaperTask->state == TASK_STATE_SIGKILLED)
      taskKill(reaperTask->id, 128 + reaperTask->tmpRecV);
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

    // free info splits
    taskInfoFsDiscard(reaperTask->infoFs);

    // interrupted syscalls
    TaskSysInterrupted *intrBrowse =
        (TaskSysInterrupted *)reaperTask->dsSysIntr.firstObject;
    while (intrBrowse) {
      TaskSysInterrupted *next = (TaskSysInterrupted *)intrBrowse->_ll.next;
      free(next); // quick n dirty
      intrBrowse = next;
    }

    // free the task now that it's safe
    taskListDestroy(reaperTask);

    // todo: free() the task in a safe manner. also implement some system for
    // editing the global LL without race conditions
    // (*) depends: task blocking, watchlists, task adding, forking, etc.

    // will need a good system for that so maybe delay it to when multithreading
    // is implemented!

    // done! continue listening
    reaperTask = 0;
  }

end:
  spinlockRelease(&LOCK_REAPER);
}

// todo: find cleaner alternatives to this and to the kernel helper as a whole
void helperVolatilePoll() {
  if (!consoleDisabled) // kernel io
    pollInstanceRing(69, EPOLLIN);
  for (int i = 0; i < lastInputEvent; i++) {
    if (atomicRead64(&devInputEvents[i].deviceEvents.readPtr) !=
        atomicRead64(&devInputEvents[i].deviceEvents.writePtr))
      pollInstanceRing((size_t)&devInputEvents[i], EPOLLIN);
  }
}

void kernelHelpEntry() {
  while (true) {
    helperNet();
    helperReaper();
    helperVolatilePoll();

    handControl();
  }
}

void initiateKernelThreads() {
  netHelperTask = taskCreateKernel((size_t)kernelHelpEntry, 0);
  taskNameKernel(netHelperTask, helperCmdline, sizeof(helperCmdline));
}
