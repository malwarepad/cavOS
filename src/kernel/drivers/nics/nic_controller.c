#include <ne2k.h>
#include <nic_controller.h>
#include <rtl8139.h>
#include <system.h>
#include <util.h>

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
  initiateRTL8139(device);
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

void sendPacket(NIC *nic, uint8_t *destination_mac, void *data, uint32_t size,
                uint16_t protocol) {
  netPacketHeader *packet = malloc(sizeof(netPacketHeader) + size);
  void            *packetData = (void *)packet + sizeof(netPacketHeader);

  memcpy(packet->source_mac, nic->MAC, 6);
  memset(packet->destination_mac, 0, 6);
  packet->ethertype = switch_endian_16(protocol);

  memcpy(packetData, data, size);

  switch (nic->type) {
  case NE2000:
    sendNe2000(nic, packet, sizeof(netPacketHeader) + size);
    break;
  case RTL8139:
    sendRTL8139(nic, packet, sizeof(netPacketHeader) + size);
    break;
  }

  free(packet);
}

void handlePacket(NIC *nic, void *packet, uint32_t size) {
  // handle packet
}
