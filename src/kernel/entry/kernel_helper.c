#include <kernel_helper.h>
#include <nic_controller.h>
#include <task.h>
#include <types.h>
#include <util.h>

// Kernel helper thread entry...
// It will help with cleaning up processes, managing networking, etc while
// adhering to spinlocks (in contrast with interrupts)
// Copyright (C) 2024 Panagiotis

void netHelperEntry() {
  while (true) {
    for (int i = 0; i < QUEUE_MAX; i++) {
      if (!netQueue[i].exists)
        continue;

      handlePacket(netQueue[i].nic, netQueue[i].buff, netQueue[i].packetLength);
      netQueue[i].exists = false;
    }
  }
}

void initiateKernelThreads() {
  // a
  taskCreateKernel((size_t)netHelperEntry, 0);
}
