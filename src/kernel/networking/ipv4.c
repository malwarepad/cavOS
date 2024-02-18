#include <checksum.h>
#include <icmp.h>
#include <ipv4.h>
#include <liballoc.h>
#include <system.h>
#include <tcp.h>
#include <udp.h>
#include <util.h>

// IP(v4) layer (https://en.wikipedia.org/wiki/Internet_Protocol_version_4)
// Copyright (C) 2023 Panagiotis

void netIPv4Send(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                 void *data, uint32_t data_size, uint8_t protocol) {
  uint32_t fullSize = sizeof(IPv4header) + data_size;
  if (fullSize > IPV4_MAX) {
    debugf("[networking::ipv4] Packet excedes limit, aborting: %d/%d\n",
           fullSize, IPV4_MAX);
    return;
  }

  uint8_t    *final = malloc(fullSize);
  IPv4header *header = (IPv4header *) final;

  memset(header, 0, sizeof(IPv4header));

  header->version = 0x04;
  header->ihl = 5; // no options required xd
  header->tos = 0;
  header->length = switch_endian_16(fullSize);
  header->id = switch_endian_16(0);
  header->flags = switch_endian_16(0);
  header->frag_offset = switch_endian_16(0);
  header->ttl = 64;
  header->protocol = protocol;

  memcpy(header->source_address, nic->ip, 4);
  memcpy(header->destination_address, destination_ip, 4);

  // calculate checksum before request's finalized
  header->checksum = checksum(header, sizeof(IPv4header));

  memcpy((uint32_t) final + sizeof(IPv4header), data, data_size);

  sendPacket(nic, destination_mac, final, fullSize, 0x0800);

  free(final);
}

bool netIPv4Verify(NIC *nic, IPv4header *header) {
  if (header->version != 0x04) {
    debugf("[networking::ipv4] Bad version, ignoring: %d\n", header->version);
    return false;
  }

  if (switch_endian_16(header->length) < 20) {
    debugf("[networking::ipv4] Too small length, ignoring: %d\n",
           switch_endian_16(header->length));
    return false;
  }

  if (*(uint32_t *)(&header->destination_address[0]) &&
      *(uint32_t *)(&header->destination_address[0]) != (uint32_t)-1 &&
      memcmp(header->destination_address, nic->ip, 4)) {
    debugf("[networking::ipv4] Bad IP, ignoring: %d:%d:%d:%d\n",
           header->destination_address[0], header->destination_address[1],
           header->destination_address[2], header->destination_address[3]);
    return false;
  }

  return true;
}

void netIPv4Receive(NIC *nic, void *body, uint32_t size) {
  IPv4header *header = (uint32_t)body + sizeof(netPacketHeader);
  if (!netIPv4Verify(nic, header))
    return;

  switch (header->protocol) {
  case ICMP_PROTOCOL:
    netICMPreceive(nic, body, size);
    break;

  case UDP_PROTOCOL:
    netUdpReceive(nic, body, size);
    break;

  case TCP_PROTOCOL:
    netTcpReceiveInternal(nic, body, size);
    break;

  default:
    debugf("[ipv4] Odd protocol: %d\n", header->protocol);
    break;
  }
}
