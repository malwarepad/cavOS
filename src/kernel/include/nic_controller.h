#include "pci.h"
#include "types.h"

#ifndef NIC_CONTROLLER_H
#define NIC_CONTROLLER_H

/* NICs */

typedef enum NIC_TYPE { NE2000, RTL8139 } NIC_TYPE;

typedef struct NIC NIC;

struct NIC {
  NIC_TYPE type;
  uint32_t infoLocation;
  uint8_t  MAC[5];
  uint8_t  irq;

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

void sendPacket(NIC *nic, uint8_t *destination_mac, void *data, uint32_t size,
                uint16_t protocol);
void handlePacket(NIC *nic, void *packet, uint32_t size);

#endif
