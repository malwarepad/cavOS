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

typedef struct tcpConnection {
  bool open;
  bool closing;
  bool closed; // officially terminated

  uint64_t lastRetransmissionTime;

  uint32_t client_seq_number;
  uint32_t client_ack_number;
} tcpConnection;

tcpConnection *netTcpConnect(NIC *nic, Socket *socket);
void           netTcpAwaitOpen(Socket *socket);

bool netTcpClose(NIC *nic, Socket *socket);
bool netTcpCleanup(NIC *nic, Socket *socket);

bool netTcpSend(NIC *nic, Socket *socket, uint8_t flags, void *data,
                uint32_t size);

void netTcpReceive(NIC *nic, void *body, uint32_t size);

void netTcpSendUnsafe(NIC *nic, Socket *socket, uint8_t flags, void *data,
                      uint32_t size);
void netTcpAck(NIC *nic, Socket *socket);
void netTcpReAck(NIC *nic, Socket *socket);
void netTcpFinishHandshake(NIC *nic, Socket *socket, void *request,
                           uint32_t size);

#endif
