#include <arp.h>
#include <checksum.h>
#include <liballoc.h>
#include <ne2k.h>
#include <system.h>
#include <timer.h>
#include <util.h>

// ARP (Address Resolution Protocol)
// Converts MAC -> IP(v4)
// Copyright (C) 2024 Panagiotis

// Xerox destination (zero'd)
uint8_t broadcastMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* Arp tables (send help) */
void registerArpTableEntry(NIC *nic, uint8_t *ip, uint8_t *mac) {
  arpTableEntry *entry = &nic->arpTable[nic->arpTableCurr++];

  memcpy(entry->ip, ip, ARP_PROTOCOL_SIZE);
  memcpy(entry->mac, mac, ARP_HARDWARE_SIZE);

  // Overwrite table from the start
  if (nic->arpTableCurr >= ARP_TABLE_LEN)
    nic->arpTableCurr = 0;
}

arpTableEntry *lookupArpTable(NIC *nic, uint8_t *ip) {
  for (int i = 0; i < ARP_TABLE_LEN; i++) {
    if (*(uint32_t *)(&nic->arpTable[i].ip[0]) == *(uint32_t *)(ip))
      return &nic->arpTable[i];
  }

  return 0; // null ptr
}

void debugArpTable(NIC *nic) {
  printf("\n");
  for (int i = 0; i < ARP_TABLE_LEN; i++) {
    if (!(*(uint32_t *)(&nic->arpTable[i].ip[0])))
      continue;

    printf("{ip: %d.%d.%d.%d, mac: %02X:%02X:%02X:%02X:%02X:%02X}\n",
           nic->arpTable[i].ip[0], nic->arpTable[i].ip[1],
           nic->arpTable[i].ip[2], nic->arpTable[i].ip[3],
           nic->arpTable[i].mac[0], nic->arpTable[i].mac[1],
           nic->arpTable[i].mac[2], nic->arpTable[i].mac[3],
           nic->arpTable[i].mac[4], nic->arpTable[i].mac[5]);
  }
  printf("\n");
}

/* The send/respond functions don't manipualte the arp table at all, that's the
 * job of the handle function, called by the generic NIC interface controller*/

void netArpSend(NIC *nic, uint8_t *ip) {
  arpPacket *arp = malloc(sizeof(arpPacket));
  memset(arp, 0, sizeof(arpPacket));

  arp->hardware_type = switch_endian_16(ARP_HARDWARE_TYPE);
  arp->protocol_type = switch_endian_16(ARP_PROTOCOL_TYPE);
  arp->hardware_size = ARP_HARDWARE_SIZE;
  arp->protocol_size = ARP_PROTOCOL_SIZE;
  arp->opcode = switch_endian_16(ARP_OP_REQUEST);

  memcpy(arp->sender_ip, nic->ip, ARP_PROTOCOL_SIZE);
  memcpy(arp->sender_mac, nic->MAC, ARP_HARDWARE_SIZE);

  memset(arp->target_mac, 0, ARP_HARDWARE_SIZE); // we don't know it!
  memcpy(arp->target_ip, ip, ARP_PROTOCOL_SIZE);

  // Send packet
  sendPacket(nic, broadcastMAC, arp, sizeof(arpPacket), NET_ETHERTYPE_ARP);

  free(arp);
}

void netArpReply(NIC *nic, arpPacket *arpRequest) {
  arpPacket *arpResponse = (arpPacket *)malloc(sizeof(arpPacket));
  memset(arpResponse, 0, sizeof(arpPacket));

  arpResponse->hardware_type = switch_endian_16(ARP_HARDWARE_TYPE);
  arpResponse->protocol_type = switch_endian_16(ARP_PROTOCOL_TYPE);
  arpResponse->hardware_size = ARP_HARDWARE_SIZE;
  arpResponse->protocol_size = ARP_PROTOCOL_SIZE;
  arpResponse->opcode = switch_endian_16(ARP_OP_REPLY);

  memcpy(arpResponse->sender_ip, nic->ip, ARP_PROTOCOL_SIZE);
  memcpy(arpResponse->sender_mac, nic->MAC, ARP_HARDWARE_SIZE);

  memcpy(arpResponse->target_mac, arpRequest->sender_mac, ARP_HARDWARE_SIZE);
  memcpy(arpResponse->target_ip, arpRequest->sender_ip, ARP_PROTOCOL_SIZE);

  // Send packet
  sendPacket(nic, arpRequest->sender_mac, arpResponse, sizeof(arpPacket),
             NET_ETHERTYPE_ARP);

  free(arpResponse);
}

void netArpHandle(NIC *nic, arpPacket *packet) {
  switch (switch_endian_16(packet->opcode)) {
  case ARP_OP_REQUEST:
    if (memcmp(packet->target_ip, nic->ip, 4) == 0)
      netArpReply(nic, packet);
    break;
  case ARP_OP_REPLY:
    // no need to reply to a response... lol.
    break;
  default:
    debugf("[networking::arp] Got an odd opcode: exact{%X} reversed{%X}\n",
           packet->opcode, switch_endian_16(packet->opcode));
    break;
  }

  /*printf("GOT SMTH!! {ip: %d.%d.%d.%d, mac: %02X:%02X:%02X:%02X:%02X:%02X}\n",
         packet->sender_ip[0], packet->sender_ip[1], packet->sender_ip[2],
         packet->sender_ip[3], packet->sender_mac[0], packet->sender_mac[1],
         packet->sender_mac[2], packet->sender_mac[3], packet->sender_mac[4],
         packet->sender_mac[5]);*/

  if (!lookupArpTable(nic, packet->sender_ip)) { // unless already stored
    // store the ip & mac regardless of request
    registerArpTableEntry(nic, packet->sender_ip, packet->sender_mac);
  }
}

// The ONLY function an end user should interact with
bool netArpGetIPv4(NIC *nic, const uint8_t *ipInput, uint8_t *mac) {
  if (memcmp(nic->ip, ipInput, 4) == 0) {
    // your own ip dummy
    memcpy(mac, nic->MAC, 6);
    return true;
  }

  const uint8_t *ip = isLocalIPv4(ipInput) ? ipInput : nic->serverIp;

  arpTableEntry *tableEntry = lookupArpTable(nic, ip);
  if (tableEntry) {
    memcpy(mac, tableEntry->mac, 6);
    return true;
  }

  netArpSend(nic, ip);

  uint64_t       caputre = timerTicks;
  arpTableEntry *tableEntryRetry = lookupArpTable(nic, ip);

  // retry until either half a second passes or we get a reply
  while (!tableEntryRetry && timerTicks < (caputre + ARP_TIMEOUT)) {
    tableEntryRetry = lookupArpTable(nic, ip);
  }

  if (tableEntryRetry) {
    memcpy(mac, tableEntryRetry->mac, 6);
    return true;
  }

  return false;
}
