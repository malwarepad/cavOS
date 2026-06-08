#include <netqueue.h>
#include <nic_controller.h>
#include <timer.h>

// Internal queue driven mechanism to avoid thread complications
// Copyright (C) 2025 Panagiotis

void netQueueMainCb(void *data, void *ctx) {
  netQueueItem *item = data;
  if (item->callback) {
    spinlockRelease(&LOCK_LL_NET_QUEUE);
    item->callback(item);
    spinlockAcquire(&LOCK_LL_NET_QUEUE);
  }
}

// every once in a while have this be ran to check various states, maintain
// timeouts, re-send stuff, etc
void helperNetQueue() {
  spinlockAcquire(&LOCK_LL_NET_QUEUE);
  LinkedListTraverse(&dsNetQueue, netQueueMainCb, 0);
  spinlockRelease(&LOCK_LL_NET_QUEUE);
}

// MAKE SURE YOU LOCK BEFORE USING THIS!
netQueueItem *netQueueAllocateM(netQueueComp component, void *nic) {
  netQueueItem *item = LinkedListAllocate(&dsNetQueue, sizeof(netQueueItem));
  item->component = component;
  item->unixTime = timerTicks;
  item->task = currentTask;
  item->nic = nic;
  return item;
}
