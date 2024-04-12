#include <ipv4.h>
#include <linked_list.h>
#include <malloc.h>
#include <socket.h>
#include <system.h>
#include <udp.h>

// Something like Linux sockets...
// todo: move TCP stuff here too
// Copyright (C) 2024 Panagiotis

Socket *netSocketConnect(NIC *nic, SOCKET_PROT protocol,
                         uint8_t *destination_ip, uint16_t source_port,
                         uint16_t destination_port) {
  if (!nic)
    return 0;
  Socket *target = LinkedListAllocate(&nic->firstSocket, sizeof(Socket));

  target->client_port = source_port;
  target->server_port = destination_port;

  if (destination_ip)
    memcpy(target->server_ip, destination_ip, 4);
  target->protocol = protocol;

  switch (protocol) {
  case SOCKET_PROT_NULL:
    debugf("[socket] FATAL! NULL protocol passed! Not supported operation!\n");
    panic();
    break;
  case SOCKET_PROT_UDP:
    // UDP doesn't require any preperation
    break;
  default:
    break;
  }

  return target;
}

void netSocketPass(NIC *nic, SOCKET_PROT protocol, void *body, uint32_t size) {
  IPv4header *ipv4 = (size_t)body + sizeof(netPacketHeader);

  uint16_t client_port = 0, server_port = 0;
  switch (protocol) {
  case SOCKET_PROT_UDP:
    udpHeader *udp =
        (size_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);
    client_port = switch_endian_16(udp->destination_port);
    server_port = switch_endian_16(udp->source_port);
    break;
  default:
    break;
  }

  if (!client_port || !server_port) {
    debugf("[socket] Cannot pass! Client{%d}/server{%d} port detection fail!\n",
           client_port, server_port);
    return;
  }

  Socket *browse = nic->firstSocket;
  while (browse) {
    if (browse->client_port == client_port)
      break;

    browse = browse->next;
  }

  if (!browse) {
    debugf("[socket] Could not match incoming packet with any sockets!\n");
    return;
  }

  if (browse->server_port && browse->server_port != server_port)
    debugf("[socket] WARNING! Incoming packet server port{%d} does NOT match "
           "captured one{%d}!",
           browse->server_port, server_port);

  if (*(uint32_t *)(&browse->server_ip[0]) &&
      memcmp(ipv4->source_address, browse->server_ip, 4) != 0)
    debugf("[socket] Incoming packet (server) IP{%d.%d.%d.%d} does not match "
           "captured (server) IP{%d.%d.%d.%d}",
           ipv4->source_address[0], ipv4->source_address[1],
           ipv4->source_address[2], ipv4->source_address[3],
           browse->server_ip[0], browse->server_ip[1], browse->server_ip[2],
           browse->server_ip[3]);

  socketPacketHeader *targetHeader = LinkedListAllocate(
      &browse->firstPacket, sizeof(socketPacketHeader) + size);
  targetHeader->size = size;
  memcpy((size_t)targetHeader + sizeof(socketPacketHeader), body, size);
}

socketPacketHeader *netSocketRecv(Socket *socket) {
  socketPacketHeader *target = socket->firstPacket;
  if (!target)
    return 0;

  size_t              totalSize = sizeof(socketPacketHeader) + target->size;
  socketPacketHeader *final = (socketPacketHeader *)malloc(totalSize);
  memcpy(final, target, totalSize);

  // First unregisters, so the interrupt handler should have no problem browsing
  // through the list...
  LinkedListRemove(&socket->firstPacket, target);
  return final;
}

void netSocketRecvCleanup(socketPacketHeader *packet) {
  // just in case
  free(packet);
}
