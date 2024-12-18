#include <checksum.h>
#include <icmp.h>
#include <malloc.h>
#include <system.h>
#include <util.h>

// Internet Control Message Protocol
// (basically just /usr/bin/ping)
// Copyright (C) 2024 Panagiotis

void netICMPsendPing(NIC *nic, uint8_t *destination_mac,
                     uint8_t *destination_ip) {
  icmpHeader header = {0};

  header.type = ICMP_ECHO;
  header.code = 0;
  header.checksum = checksum(&header, sizeof(icmpHeader));

  netIPv4Send(nic, destination_mac, destination_ip, &header, sizeof(icmpHeader),
              ICMP_PROTOCOL);
}

void netICMPreply(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                  icmpHeader *requestHeader, void *body, uint32_t body_size) {
  uint32_t    finalSize = sizeof(icmpHeader) + body_size;
  uint8_t    *final = malloc(finalSize);
  icmpHeader *header = (icmpHeader *) final;

  memset(final, 0, finalSize);

  header->type = ICMP_REPLY;
  header->code = 0;

  header->restOfHeader = requestHeader->restOfHeader;
  memcpy((void *)((size_t) final + sizeof(icmpHeader)), body, body_size);
  header->checksum = checksum(final, finalSize);

  netIPv4Send(nic, destination_mac, destination_ip, final, finalSize,
              ICMP_PROTOCOL);

  free(final);
}

void netICMPreceive(NIC *nic, void *packet, uint32_t size) {
  netPacketHeader *rawHeader = packet;
  IPv4header *ipv4 = (IPv4header *)((size_t)packet + sizeof(netPacketHeader));
  icmpHeader *icmp = (icmpHeader *)((size_t)ipv4 + sizeof(IPv4header));
  void       *icmpBody = (void *)((size_t)icmp + sizeof(icmpHeader));

  switch (icmp->type) {
  case ICMP_ECHO:
    netICMPreply(nic, rawHeader->source_mac, ipv4->source_address, icmp,
                 icmpBody,
                 size - sizeof(netPacketHeader) - sizeof(IPv4header) -
                     sizeof(icmpHeader));
    break;

  case ICMP_REPLY:
    debugf("[networking::icmp::response] from{%d.%d.%d.%d} to{%d.%d.%d.%d}!\n",
           ipv4->source_address[0], ipv4->source_address[1],
           ipv4->source_address[2], ipv4->source_address[3],
           ipv4->destination_address[0], ipv4->destination_address[1],
           ipv4->destination_address[2], ipv4->destination_address[3]);
    break;
  case ICMP_DST_UNREACHABLE:
    debugf("[networking::icmp::response] unreachable!\n");
  default:
    break;
  }
}
