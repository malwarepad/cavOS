#include "pci.h"
#include "types.h"

#ifndef NIC_CONTROLLER_H
#define NIC_CONTROLLER_H

/* NICs */

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

/* Packets */
typedef struct netPacketHeader {
  uint8_t  destination_mac[6];
  uint8_t  source_mac[6];
  uint16_t ethertype;
} __attribute__((packed)) netPacketHeader;

typedef struct netPacket {
  netPacketHeader header;
  void           *data;
} netPacket;

#endif
