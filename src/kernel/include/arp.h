#include "types.h"

#ifndef ARP_H
#define ARP_H

typedef struct arpPacket {
  uint16_t hardware_type;
  uint16_t protocol_type;
  uint8_t  hardware_size;
  uint8_t  protocol_size;
  uint16_t opcode;
  uint8_t  sender_mac[6];
  uint8_t  sender_ip[4];
  uint8_t  target_mac[6];
  uint8_t  target_ip[4];
} __attribute__((packed)) arpPacket;

enum ARPOperation {
  ARP_OP_REQUEST = 0x01,
  ARP_OP_REPLY = 0x02,
};

void testArpBroadcast();

#endif