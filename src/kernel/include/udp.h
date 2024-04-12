#include "nic_controller.h"
#include "types.h"

#ifndef UDP_H
#define UDP_H

#define UDP_PROTOCOL 17

typedef struct udpHeader {
  uint16_t source_port;
  uint16_t destination_port;
  uint16_t length;
  uint16_t checksum;
} __attribute__((packed)) udpHeader;

void netUdpSend(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                void *data, uint32_t data_size, uint16_t source_port,
                uint16_t destination_port);
void netUdpReceive(NIC *nic, void *body, uint32_t size);

#endif
