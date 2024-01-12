#include <checksum.h>
#include <icmp.h>
#include <ipv4.h>
#include <liballoc.h>
#include <system.h>
#include <util.h>

// IP(v4) layer (https://en.wikipedia.org/wiki/Internet_Protocol_version_4)
// Copyright (C) 2023 Panagiotis

void netIPv4Send(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                 void *data, uint32_t data_size, uint8_t protocol) {

  uint8_t    *final = malloc(sizeof(IPv4header) + data_size);
  IPv4header *header = (IPv4header *) final;

  memset(header, 0, sizeof(IPv4header));

  header->version = 0x04;
  header->ihl = 5; // no options required xd
  header->tos = 0;
  header->length = switch_endian_16(sizeof(IPv4header) + data_size);
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

  sendPacket(nic, destination_mac, final, sizeof(IPv4header) + data_size,
             0x0800);

  free(final);
}

void netIPv4Receive(NIC *nic, void *body, uint32_t size) {
  IPv4header *header = (uint32_t)body + sizeof(netPacketHeader);
  switch (header->protocol) {
  case ICMP_PROTOCOL:
    netICMPreceive(nic, body, size);
    break;

  default:
    debugf("[ipv4] Odd protocol: %d\n", header->protocol);
    break;
  }
}
