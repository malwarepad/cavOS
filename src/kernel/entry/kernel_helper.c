#include <kernel_helper.h>
#include <nic_controller.h>
#include <system.h>
#include <task.h>
#include <types.h>
#include <util.h>

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

void kernelHelpEntry() {
  while (true) {
    helperNet();

    handControl();
  }
}

void initiateKernelThreads() {
  netHelperTask = taskCreateKernel((size_t)kernelHelpEntry, 0);
}
