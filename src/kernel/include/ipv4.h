#include "util.h"

#ifndef NET_IPV4_H
#define NET_IPV4_H

typedef struct IPv4header {
  uint8_t ihl : 4;
  uint8_t version : 4; // network endian switch, version should be first

  uint8_t  tos;
  uint16_t length;
  uint16_t id;

  uint16_t flags;

  uint8_t  ttl;
  uint8_t  protocol;
  uint16_t checksum;
  uint8_t  srcAddress[4];
  uint8_t  destAddress[4];
} __attribute__((packed)) IPv4header;

#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_TCP 6
#define IPV4_PROTOCOL_UDP 17

#define IPV4_FLAGS_MORE_FRAGMENTS (1 << 13)
#define IPV4_FLAGS_DONT_FRAGMENT (1 << 14)
#define IPV4_FLAGS_RESV (1 << 15)

#define IPV4_FRAGMENT_OFFSET(a)                                                \
  (a &                                                                         \
   ~(IPV4_FLAGS_DONT_FRAGMENT | IPV4_FLAGS_MORE_FRAGMENTS | IPV4_FLAGS_RESV))

#define NET_IPv4_CARRY (sizeof(ethHeader))
#define NET_IPv4(packet) ((IPv4header *)((size_t)(packet) + NET_IPv4_CARRY))

void netIPv4Handle(void *_nic, void *packet, uint32_t size);
bool netIPv4NeedsRouting(uint8_t *localIp, uint8_t *destIp,
                         uint8_t *subnetMask);

void netIPv4InitBuffer(void *buffer, uint32_t size);
void netIPv4Send(void *_nic, void *packet, uint32_t size, uint8_t protocol,
                 uint8_t *destinationIp);

#endif
