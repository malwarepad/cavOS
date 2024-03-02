#include <arp.h>
#include <checksum.h>
#include <ipv4.h>
#include <liballoc.h>
#include <system.h>
#include <tcp.h>
#include <util.h>

// An actual TCP implementation! (kinda)
// Tried to keep the code as simple as I could, good for educational purposes
// Copyright (C) 2024 Panagiotis

/* Only linked-list utilities (CANNOT be used by themselves) */
bool netTcpRemoveChain(NIC *nic, uint16_t port) {
  tcpConnection *curr = nic->firstTcpConnection;
  while (curr) {
    if (curr->next && (tcpConnection *)(curr->next)->client_port == port)
      break;
    curr = curr->next;
  }
  if (nic->firstTcpConnection && nic->firstTcpConnection->client_port == port) {
    tcpConnection *target = nic->firstTcpConnection;
    nic->firstTcpConnection = target->next;
    free(target);
    return true;
  } else if (!curr)
    return false;

  tcpConnection *target = curr->next;
  curr->next = target->next; // remove reference
  free(target);              // free remaining memory

  return true;
}

// Register UNINITIATED tcp connection! (should NEVER be used alone)
tcpConnection *netTcpRegisterChain(NIC *nic, uint16_t client_port) {
  netTcpRemoveChain(nic, client_port); // ensure no old ones left

  tcpConnection *handler = (tcpConnection *)malloc(sizeof(tcpConnection));
  memset(handler, 0, sizeof(tcpConnection));
  tcpConnection *curr = nic->firstTcpConnection;
  while (1) {
    if (curr == 0) {
      // means this is our first one
      nic->firstTcpConnection = handler;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = handler;
      break;
    }
    curr = curr->next; // cycle
  }

  handler->client_port = client_port;
  handler->open = false;
  handler->client_ack_number = 0;
  handler->client_seq_number = rand();
  handler->next = 0; // null ptr
  return handler;
}

// Add packet to linked list of packet received
tcpPacketHeader *netTcpAddPacket(NIC *nic, tcpConnection *connection,
                                 void *base, uint32_t size) {
  tcpPacketHeader *packet =
      (tcpPacketHeader *)malloc(sizeof(tcpPacketHeader) + size);
  tcpPacketHeader *curr = connection->firstPendingPacket;
  while (1) {
    if (curr == 0) {
      // means this is our first one
      connection->firstPendingPacket = packet;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = packet;
      break;
    }
    curr = curr->next; // cycle
  }

  packet->size = size;
  packet->next = 0;
  memcpy((size_t)packet + sizeof(tcpPacketHeader), base, size);
  return packet;
}
/* End linked-list utilities */

// Discard packet, after reading it ofc (user-executable)
void netTcpDiscardPacket(tcpConnection *connection, tcpPacketHeader *header) {
  tcpPacketHeader *curr = connection->firstPendingPacket;
  while (curr) {
    if (curr->next == header)
      break;
    curr = curr->next;
  }
  if (connection->firstPendingPacket == header) {
    connection->firstPendingPacket = header->next;
    free(header);
    return;
  } else if (!curr) {
    free(header);
    return;
  }

  curr->next = header->next; // remove reference
  free(header);              // free remaining memory
}

void netTcpReceiveInternal(NIC *nic, void *body, uint32_t size) {
  tcpHeader *header =
      (size_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);
  IPv4header *ipv4 = (size_t)body + sizeof(netPacketHeader);

  tcpConnection *browse = nic->firstTcpConnection;
  while (browse) {
    if (browse->client_port == switch_endian_16(header->destination_port))
      break;
    browse = browse->next;
  }
  if (!browse || memcmp(browse->server_ip, ipv4->source_address, 4) != 0 ||
      browse->server_port != switch_endian_16(header->source_port))
    return;

  uint32_t tcpSize =
      (switch_endian_16(ipv4->length) - sizeof(IPv4header) - sizeof(tcpHeader));
  browse->client_ack_number +=
      switch_endian_16(ipv4->length) - sizeof(IPv4header) - sizeof(tcpHeader);

  if (header->flags & ACK_FLAG && header->flags & SYN_FLAG && !browse->open) {
    // still haven't completed handshake
    netTcpFinishHandshake(nic, body, size, browse);
  } else if (header->flags & FIN_FLAG) {
    // closure
    if (browse->closing) {
      browse->client_seq_number++;
      browse->client_ack_number++;
      netTcpAck(nic, browse);
    } else {
      browse->client_ack_number++;
      netTcpSendUnsafe(nic, browse, ACK_FLAG | FIN_FLAG, 0, 0);
    }

    browse->closing = false;
    browse->open = false;
  } else if (header->flags & ACK_FLAG && tcpSize) {
    // casual data receive
    netTcpAck(nic, browse);
    netTcpAddPacket(nic, browse, header, tcpSize + sizeof(tcpHeader));
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
void netTcpSendUnsafe(NIC *nic, tcpConnection *connection, uint8_t flags,
                      void *data, uint32_t size) {
  uint8_t mac[6];
  netArpGetIPv4(nic, connection->server_ip, mac);

  netTcpSendGeneric(nic, connection->server_ip, mac, connection->client_port,
                    connection->server_port, connection->client_seq_number,
                    connection->client_ack_number, flags, data, size);

  connection->client_seq_number += size;
}

void netTcpAck(NIC *nic, tcpConnection *connection) {
  netTcpSendUnsafe(nic, connection, ACK_FLAG, 0, 0);
}

void netTcpFinishHandshake(NIC *nic, void *request, uint32_t size,
                           tcpConnection *connection) {
  tcpHeader *tcpReq =
      (size_t)request + sizeof(netPacketHeader) + sizeof(IPv4header);

  connection->client_ack_number = switch_endian_32(tcpReq->sequence_number);

  uint8_t mac[6];
  netArpGetIPv4(nic, connection->server_ip, mac);
  netTcpSendGeneric(nic, connection->server_ip, mac, connection->client_port,
                    connection->server_port, ++connection->client_seq_number,
                    ++connection->client_ack_number, ACK_FLAG, 0, 0);

  connection->open = true; // hell yea
}

/* Most below functions are usual and user-usable half-securely (lol) */

tcpConnection *netTcpConnect(NIC *nic, uint8_t *destination_ip,
                             uint16_t source_port, uint16_t destination_port) {
  // Start the threeway (handshake... IT'S A HANDSHAKE!)
  tcpConnection *connection = netTcpRegisterChain(nic, source_port);

  connection->open = false;
  connection->closing = false;
  connection->client_port = source_port;
  connection->server_port = destination_port;
  connection->firstPendingPacket = 0; // ain't got shit atm
  memcpy(connection->server_ip, destination_ip, 4);

  uint8_t mac[6];
  netArpGetIPv4(nic, destination_ip, mac);
  netTcpSendGeneric(nic, destination_ip, mac, source_port, destination_port,
                    connection->client_seq_number,
                    connection->client_ack_number, SYN_FLAG, 0, 0);
  return connection;
}

bool netTcpSend(NIC *nic, tcpConnection *connection, uint8_t flags, void *data,
                uint32_t size) {
  if (!connection->open || connection->closing)
    return false;
  netTcpSendUnsafe(nic, connection, flags, data, size);
  return true;
}

void netTcpAwaitOpen(tcpConnection *connection) {
  if (connection->closing)
    return;
  while (!connection->open)
    ;
}

bool netTcpClose(NIC *nic, tcpConnection *connection) {
  if (!connection->open || connection->closing)
    return false;

  connection->closing = true;
  netTcpSendUnsafe(nic, connection, FIN_FLAG, 0, 0);
  return true;
}

bool netTcpCleanup(NIC *nic, tcpConnection *connection) {
  if (connection->open)
    return false;

  while (connection->firstPendingPacket)
    netTcpDiscardPacket(connection, connection->firstPendingPacket);
  netTcpRemoveChain(nic, connection->client_port);
  return true;
}

netPacketHeader *netTcpReceive(tcpConnection *connection) {
  // first is really the last not acknowledged
  return connection->firstPendingPacket;
}
