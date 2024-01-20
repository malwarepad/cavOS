#include "nic_controller.h"
#include "types.h"

#ifndef TCP_H
#define TCP_H

#define TCP_PROTOCOL 6

//                           CEUAPRSF
#define CWR_FLAG (1 << 7) // 10000000
#define ECE_FLAG (1 << 6) // 01000000
#define URG_FLAG (1 << 5) // 00100000
#define ACK_FLAG (1 << 4) // 00010000
#define PSH_FLAG (1 << 3) // 00001000
#define RST_FLAG (1 << 2) // 00000100
#define SYN_FLAG (1 << 1) // 00000010
#define FIN_FLAG (1 << 0) // 00000001

typedef struct tcpHeader {
  uint16_t source_port;
  uint16_t destination_port;
  uint32_t sequence_number;
  uint32_t acknowledgement_number;
  uint8_t  data_offset; // 4 bits (+ 4 of reserved)
  uint8_t  flags;
  uint16_t window_size;
  uint16_t checksum;
  uint16_t urgent_ptr;
} __attribute__((packed)) tcpHeader;

tcpConnection *netTcpConnect(NIC *nic, uint8_t *destination_ip,
                             uint16_t source_port, uint16_t destination_port);
void           netTcpReceiveInternal(NIC *nic, void *body, uint32_t size);
bool netTcpSend(NIC *nic, tcpConnection *connection, uint8_t flags, void *data,
                uint32_t size);

void netTcpAwaitOpen(tcpConnection *connection);

void netTcpDiscardPacket(tcpConnection *connection, tcpPacketHeader *header);
bool netTcpClose(NIC *nic, tcpConnection *connection);

bool             netTcpCleanup(NIC *nic, tcpConnection *connection);
netPacketHeader *netTcpReceive(tcpConnection *connection);

#endif
