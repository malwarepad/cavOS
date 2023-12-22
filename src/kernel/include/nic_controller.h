#include "pci.h"
#include "types.h"

#ifndef NIC_CONTROLLER_H
#define NIC_CONTROLLER_H

typedef enum NIC_TYPE { NE2000 } NIC_TYPE;

typedef struct NIC NIC;

struct NIC {
  NIC_TYPE type;
  uint32_t infoLocation;
  uint8_t  MAC[5];

  NIC *next;
};

NIC *firstNIC;
NIC *selectedNIC;

void initiateNetworking();

// returns UNINITIALIZED!! NIC struct
void initiateNIC(PCIdevice *device);
NIC *createNewNIC();

#endif
