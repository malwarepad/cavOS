#include "ipv4.h"
#include "nic_controller.h"
#include "types.h"

#ifndef ICMP_H
#define ICMP_H

typedef struct icmpHeader {
  uint8_t  type;
  uint8_t  code;
  uint16_t checksum;
  uint32_t restOfHeader;
} icmpHeader;

typedef enum ICMP_TYPE {
  ICMP_REPLY = 0x00,
  ICMP_DST_UNREACHABLE = 0x03,
  ICMP_SRC_QUENCH = 0x04,
  ICMP_REDIRECT = 0x05,
  ICMP_ECHO = 0x08,
  ICMP_ROUTER_ADV = 0x09,
  ICMP_ROUTER_SOL = 0x0a,
  ICMP_TIMEOUT = 0x0b,
  ICMP_MALFORMED = 0x0c,
} ICMP_TYPE;

#define ICMP_PROTOCOL 1

void netICMPsendPing(NIC *nic, uint8_t *destination_mac,
                     uint8_t *destination_ip);
void netICMPreceive(NIC *nic, IPv4header *ipv4, icmpHeader *icmp, void *data);

#endif
