#include <arp.h>
#include <dhcp.h>
#include <ipv4.h>
#include <linked_list.h>
#include <malloc.h>
#include <ne2k.h>
#include <nic_controller.h>
#include <rtl8139.h>
#include <rtl8169.h>
#include <system.h>
#include <util.h>

// Manager for all connected network interfaces
// Copyright (C) 2024 Panagiotis

void initiateNetworking() {
  // start off with no first NIC and no selected one
  // rest on device-specific initialization
  firstNIC = 0;
  selectedNIC = 0;
  debugf("[networking] Ready to scan for NICs..\n");
}

void initiateNIC(PCIdevice *device) {
  if (initiateNe2000(device) || initiateRTL8139(device) ||
      initiateRTL8169(device)) {
    netDHCPinit(selectedNIC); // selectedNIC = newly created NIC structure
  }
}

// returns UNINITIALIZED!! NIC struct
NIC *createNewNIC() {
  NIC *nic = LinkedListAllocate(&firstNIC, sizeof(NIC));

  nic->dhcpTransactionID = rand();
  nic->mtu = 1500;
  selectedNIC = nic;
  return nic;
}

void sendPacket(NIC *nic, uint8_t *destination_mac, void *data, uint32_t size,
                uint16_t protocol) {
  netPacketHeader *packet = malloc(sizeof(netPacketHeader) + size);
  void            *packetData = (void *)packet + sizeof(netPacketHeader);

  memcpy(packet->source_mac, nic->MAC, 6);
  memcpy(packet->destination_mac, destination_mac, 6);
  packet->ethertype = switch_endian_16(protocol);

  memcpy(packetData, data, size);

  switch (nic->type) {
  case NE2000:
    sendNe2000(nic, packet, sizeof(netPacketHeader) + size);
    break;
  case RTL8139:
    sendRTL8139(nic, packet, sizeof(netPacketHeader) + size);
    break;
  case RTL8169:
    sendRTL8169(nic, packet, sizeof(netPacketHeader) + size);
    break;
  }

  free(packet);
}

void handlePacket(NIC *nic, void *packet, uint32_t size) {
  netPacketHeader *header = (netPacketHeader *)packet;
  void            *body = (void *)((size_t)packet + sizeof(netPacketHeader));

  if (memcmp(header->destination_mac, nic->MAC, 6) != 0 &&
      memcmp(header->destination_mac, macBroadcast, 6) != 0 &&
      memcmp(header->destination_mac, macZero, 6) != 0) {
    debugf("[nics] Packet isn't intended for us, ignoring! "
           "dest{%02X:%02X:%02X:%02X:%02X:%02X}\n",
           header->destination_mac[0], header->destination_mac[1],
           header->destination_mac[2], header->destination_mac[3],
           header->destination_mac[4], header->destination_mac[5]);
    return;
  }

  switch (switch_endian_16(header->ethertype)) {
  case NET_ETHERTYPE_ARP:
    netArpHandle(nic, body);
    break;
  case NET_ETHERTYPE_IPV4:
    netIPv4Receive(nic, packet, size);
    break;
  case NET_ETHERTYPE_IPV6:
    // fuck IPv6
    break;
  default:
    debugf("[nics] Odd ethertype: exact{%X} reversed{%X}\n", header->ethertype,
           switch_endian_16(header->ethertype));
    break;
  }
}
