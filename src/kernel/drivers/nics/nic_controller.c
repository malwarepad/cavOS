#include <ne2k.h>
#include <nic_controller.h>

// Manager for all connected network interfaces
// Copyright (C) 2023 Panagiotis

void initiateNetworking() {
  // start off with no first NIC and no selected one
  // rest on device-specific initialization
  firstNIC = 0;
  selectedNIC = 0;
}

void initiateNIC(PCIdevice *device) {
  initiateNe2000(device);
  // ill add more NICs in the future
  // (lie)
}

// returns UNINITIALIZED!! NIC struct
NIC *createNewNIC() {
  NIC *nic = (NIC *)malloc(sizeof(NIC));
  NIC *curr = firstNIC;
  while (1) {
    if (curr == 0) {
      // means this is our first one
      firstNIC = nic;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = nic;
      break;
    }
    curr = curr->next; // cycle
  }
  selectedNIC = nic;
  return nic;
}
