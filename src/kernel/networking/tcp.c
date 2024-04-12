#include <arp.h>
#include <checksum.h>
#include <ipv4.h>
#include <malloc.h>
#include <socket.h>
#include <system.h>
#include <tcp.h>
#include <util.h>

// An actual TCP implementation! (kinda)
// Tried to keep the code as simple as I could, good for educational purposes
// Copyright (C) 2024 Panagiotis

void netTcpReceive(NIC *nic, void *body, uint32_t size) {
  tcpHeader *header =
      (size_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);
  IPv4header *ipv4 = (size_t)body + sizeof(netPacketHeader);

  Socket *browse = nic->firstSocket;
  while (browse) {
    if (browse->protocol == SOCKET_PROT_TCP &&
        browse->client_port == switch_endian_16(header->destination_port))
      break;
    browse = browse->next;
  }
  if (!browse || !browse->protocolSpecific ||
      memcmp(browse->server_ip, ipv4->source_address, 4) != 0 ||
      browse->server_port != switch_endian_16(header->source_port))
    return;

  tcpConnection *tcp = (tcpConnection *)browse->protocolSpecific;
  uint32_t       tcpSize =
      (switch_endian_16(ipv4->length) - sizeof(IPv4header) - sizeof(tcpHeader));
  tcp->client_ack_number +=
      switch_endian_16(ipv4->length) - sizeof(IPv4header) - sizeof(tcpHeader);

  if (header->flags & ACK_FLAG && header->flags & SYN_FLAG && !tcp->open) {
    // still haven't completed handshake
    netTcpFinishHandshake(nic, browse, body, size);
  } else if (header->flags & FIN_FLAG) {
    // closure
    if (tcp->closing) {
      tcp->client_seq_number++;
      tcp->client_ack_number++;
      netTcpAck(nic, browse);
    } else {
      tcp->client_ack_number++;
      netTcpSendUnsafe(nic, browse, ACK_FLAG | FIN_FLAG, 0, 0);
    }

    tcp->closing = false;
    tcp->open = false;
  } else if (header->flags & ACK_FLAG && tcpSize) {
    // casual data receive
    netTcpAck(nic, browse);
    netSocketPass(nic, SOCKET_PROT_TCP, body, size);
  }

  // browse->handler(nic, body, size, browse); // nah
}

/* Generic function (doesn't increment ACK or SEQ)
 * no payload: payload=0, size=0
 */
void netTcpSendGeneric(NIC *nic, uint8_t *destination_ip,
                       uint8_t *destination_mac, uint32_t source_port,
                       uint32_t destination_port, uint32_t sequence_number,
                       uint32_t acknowledgement_number, uint8_t flags,
                       void *payload, uint32_t size) {
  uint8_t   *body = (uint8_t *)malloc(sizeof(tcpHeader) + size);
  tcpHeader *header = (tcpHeader *)body;

  header->source_port = switch_endian_16(source_port);
  header->destination_port = switch_endian_16(destination_port);

  header->sequence_number = switch_endian_32(sequence_number);
  header->acknowledgement_number = switch_endian_32(acknowledgement_number);

  header->data_offset = (sizeof(tcpHeader) / 4) << 4;
  header->flags = flags;

  header->window_size = switch_endian_16(64440);
  header->checksum = 0;
  header->urgent_ptr = 0;

  memcpy((size_t)header + sizeof(tcpHeader), payload, size);

  header->checksum =
      tcpChecksum(body, sizeof(tcpHeader) + size, nic->ip, destination_ip);

  netIPv4Send(nic, destination_mac, destination_ip, body,
              sizeof(tcpHeader) + size, TCP_PROTOCOL);
  free(body);
}

/* More caring, still not secure!
 * (will not check if connection is ready)
 */
void netTcpSendUnsafe(NIC *nic, Socket *socket, uint8_t flags, void *data,
                      uint32_t size) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  netTcpSendGeneric(nic, socket->server_ip, socket->server_mac,
                    socket->client_port, socket->server_port,
                    connection->client_seq_number,
                    connection->client_ack_number, flags, data, size);

  connection->client_seq_number += size;
}

void netTcpAck(NIC *nic, Socket *socket) {
  netTcpSendUnsafe(nic, socket, ACK_FLAG, 0, 0);
}

void netTcpFinishHandshake(NIC *nic, Socket *socket, void *request,
                           uint32_t size) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  tcpHeader     *tcpReq =
      (size_t)request + sizeof(netPacketHeader) + sizeof(IPv4header);

  connection->client_ack_number = switch_endian_32(tcpReq->sequence_number);

  netTcpSendGeneric(nic, socket->server_ip, socket->server_mac,
                    socket->client_port, socket->server_port,
                    ++connection->client_seq_number,
                    ++connection->client_ack_number, ACK_FLAG, 0, 0);

  connection->open = true; // hell yea
}

/* Most below functions are usual and user-usable half-securely (lol) */

tcpConnection *netTcpConnect(NIC *nic, Socket *socket) {
  // Start the threeway (handshake... IT'S A HANDSHAKE!)
  tcpConnection *connection = (tcpConnection *)malloc(sizeof(tcpConnection));

  connection->open = false;
  connection->closing = false;
  // connection->client_port = source_port;
  // connection->server_port = destination_port;
  // memcpy(socket->server_ip, destination_ip, 4);

  netTcpSendGeneric(nic, socket->server_ip, socket->server_mac,
                    socket->client_port, socket->server_port,
                    connection->client_seq_number,
                    connection->client_ack_number, SYN_FLAG, 0, 0);
  return connection;
}

bool netTcpSend(NIC *nic, Socket *socket, uint8_t flags, void *data,
                uint32_t size) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  if (!connection->open || connection->closing)
    return false;
  netTcpSendUnsafe(nic, socket, flags, data, size);
  return true;
}

void netTcpAwaitOpen(Socket *socket) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  if (connection->closing)
    return;
  while (!connection->open)
    ;
}

bool netTcpClose(NIC *nic, Socket *socket) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  if (!connection->open || connection->closing)
    return false;

  connection->closing = true;
  netTcpSendUnsafe(nic, socket, FIN_FLAG, 0, 0);
  return true;
}

bool netTcpCleanup(NIC *nic, Socket *socket) {
  tcpConnection *connection = (tcpConnection *)socket->protocolSpecific;
  if (connection->open)
    return false;

  free(connection);
  return true;
}
