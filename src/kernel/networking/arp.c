#include <arp.h>
#include <liballoc.h>
#include <ne2k.h>
#include <system.h>
#include <util.h>

// ARP (Address Resolution Protocol)
// Converts MAC -> IP(v4)
// Copyright (C) 2023 Panagiotis

uint8_t emptyMAC[6] = {0, 0, 0, 0, 0, 0};

// DO NOT USE THIS YET!
// (just a test ARP broadcast)
void testArpBroadcast() {
  NIC *nic = selectedNIC;
  if (!nic)
    return;

  arpPacket *arp = malloc(sizeof(arpPacket));

  arp->hardware_type = switch_endian_16(0x1);
  arp->protocol_type = switch_endian_16(0x0800);
  arp->hardware_size = 6;
  arp->protocol_size = 4;
  arp->opcode = switch_endian_16(ARP_OP_REQUEST);

  for (int i = 0; i < 6; i++) {
    arp->sender_mac[i] = nic->MAC[i];
    arp->target_mac[i] = 0;
  }

  arp->sender_ip[0] = 10;
  arp->sender_ip[1] = 0;
  arp->sender_ip[2] = 2;
  arp->sender_ip[3] = 0;

  arp->target_ip[0] = 192;
  arp->target_ip[1] = 168;
  arp->target_ip[2] = 2;
  arp->target_ip[3] = 5;

  // Send packet
  printf("sending...\n");
  sendPacket(nic, emptyMAC, arp, sizeof(arpPacket), 0x0806);

  free(arp);
  printf("sent!\n");
}
