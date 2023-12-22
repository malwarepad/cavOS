#include "pci.h"
#include "types.h"

#ifndef NE2K_H
#define NE2K_H

typedef struct ne2k_interface {
} ne2k_interface;

bool initiateNe2000(PCIdevice *device);

#endif