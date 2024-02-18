#include "nic_controller.h"
#include "types.h"

#ifndef IPV4_H
#define IPV4_H

#define IPV4_MAX 65535

typedef struct IPv4header {
  uint8_t ihl : 4;
  uint8_t version : 4; // network endian switch, version should be first

  uint8_t  tos;
  uint16_t length;
  uint16_t id;

  uint16_t frag_offset : 13;
  uint16_t flags : 3; // same ugly network endian switch

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
