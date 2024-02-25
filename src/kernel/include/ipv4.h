#include "nic_controller.h"
#include "types.h"

#ifndef IPV4_H
#define IPV4_H

#define IPV4_MAX 65535

// little endian, end-to-start, 16 bit masks
#define IPV4_FLAGS_DONT_FRAGMENT (1 << 12)
#define IPV4_FLAGS_MORE_FRAGMENTS (1 << 13)
#define IPV4_FLAGS_RESV (1 << 14)

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
  uint8_t  source_address[4];
  uint8_t  destination_address[4];
} __attribute__((packed)) IPv4header;

void netIPv4Send(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                 void *data, uint32_t data_size, uint8_t protocol);
void netIPv4Receive(NIC *nic, void *body, uint32_t size);

#endif
