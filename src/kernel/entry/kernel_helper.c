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

void netHelperEntry() {
  while (true) {
    while (netQueueRead == netQueueWrite) {
      // empty :p
      handControl();
    }

    handlePacket(netQueue[netQueueRead].nic, netQueue[netQueueRead].buff,
                 netQueue[netQueueRead].packetLength);
    netQueueRead = (netQueueRead + 1) % QUEUE_MAX;
  }
}

void initiateKernelThreads() {
  netHelperTask = taskCreateKernel((size_t)netHelperEntry, 0);
}
