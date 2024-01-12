#include <checksum.h>
#include <icmp.h>
#include <liballoc.h>
#include <system.h>
#include <util.h>

// Internet Control Message Protocol
// (basically just /usr/bin/ping)
// Copyright (C) 2023 Panagiotis

void netICMPsendPing(NIC *nic, uint8_t *destination_mac,
                     uint8_t *destination_ip) {
  uint8_t    *final = malloc(sizeof(icmpHeader));
  icmpHeader *header = (icmpHeader *) final;

  memset(header, 0, sizeof(icmpHeader));

  header->type = ICMP_ECHO;
  header->code = 0;
  header->checksum = checksum(header, sizeof(header));

  netIPv4Send(nic, destination_mac, destination_ip, final, sizeof(icmpHeader),
              ICMP_PROTOCOL);

  free(final);
}

void netICMPreply(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip) {
  uint8_t    *final = malloc(sizeof(icmpHeader));
  icmpHeader *header = (icmpHeader *) final;

  memset(header, 0, sizeof(icmpHeader));

  header->type = ICMP_REPLY;
  header->code = 0;
  header->checksum = checksum(header, sizeof(header));

  netIPv4Send(nic, destination_mac, destination_ip, final, sizeof(icmpHeader),
              ICMP_PROTOCOL);

  free(final);
}

void netICMPreceive(NIC *nic, void *packet, uint32_t size) {
  netPacketHeader *rawHeader = packet;
  IPv4header      *ipv4 = (uint32_t)packet + sizeof(netPacketHeader);
  icmpHeader      *icmp = (uint32_t)ipv4 + sizeof(IPv4header);

  switch (icmp->type) {
  case ICMP_ECHO:
    netICMPreply(nic, rawHeader->source_mac, ipv4->source_address);
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
