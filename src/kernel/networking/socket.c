#include <arp.h>
#include <ipv4.h>
#include <linked_list.h>
#include <malloc.h>
#include <socket.h>
#include <system.h>
#include <tcp.h>
#include <udp.h>

// Something like Linux sockets...
// Copyright (C) 2024 Panagiotis

Socket *netSocketConnect(NIC *nic, SOCKET_PROT protocol,
                         uint8_t *destination_ip, uint16_t source_port,
                         uint16_t destination_port) {
  if (!nic)
    return 0;
  Socket *target =
      LinkedListAllocate((void **)&nic->firstSocket, sizeof(Socket));

  target->client_port = source_port;
  target->server_port = destination_port;

  if (destination_ip) {
    memcpy(target->server_ip, destination_ip, 4);
    netArpGetIPv4(nic, destination_ip, target->server_mac);
  }
  target->protocol = protocol;

  switch (protocol) {
  case SOCKET_PROT_NULL:
    debugf("[socket] FATAL! NULL protocol passed! Not supported operation!\n");
    panic();
    break;
  case SOCKET_PROT_UDP:
    // UDP doesn't require any preperation
    break;
  case SOCKET_PROT_TCP: {
    tcpConnection *tcp = netTcpConnect(nic, target);
    target->protocolSpecific = tcp;
    break;
  }
  default:
    break;
  }

  return target;
}

bool netSocketPass(NIC *nic, SOCKET_PROT protocol, void *body, uint32_t size) {
  IPv4header *ipv4 = (IPv4header *)((size_t)body + sizeof(netPacketHeader));

  uint16_t client_port = 0, server_port = 0;
  switch (protocol) {
  case SOCKET_PROT_UDP: {
    udpHeader *udp = (udpHeader *)((size_t)body + sizeof(netPacketHeader) +
                                   sizeof(IPv4header));
    client_port = switch_endian_16(udp->destination_port);
    server_port = switch_endian_16(udp->source_port);

    int various =
        sizeof(netPacketHeader) + sizeof(IPv4header) + sizeof(udpHeader);
    body = (void *)((size_t)body + various);
    size -= various;
    break;
  }
  case SOCKET_PROT_TCP: {
    tcpHeader *tcp = (tcpHeader *)((size_t)body + sizeof(netPacketHeader) +
                                   sizeof(IPv4header));
    client_port = switch_endian_16(tcp->destination_port);
    server_port = switch_endian_16(tcp->source_port);

    int various =
        sizeof(netPacketHeader) + sizeof(IPv4header) + sizeof(tcpHeader);
    body = (void *)((size_t)body + various);
    size -= various;
    break;
  }
  default:
    break;
  }

  if (!client_port || !server_port) {
    debugf("[socket] Cannot pass! Client{%d}/server{%d} port detection fail!\n",
           client_port, server_port);
    return false;
  }

  Socket *browse = nic->firstSocket;
  while (browse) {
    if (browse->protocol == protocol && browse->client_port == client_port)
      break;

    browse = browse->next;
  }

  if (!browse) {
    debugf("[socket] Could not match incoming packet with any sockets!\n");
    return false;
  }

  if (browse->server_port && browse->server_port != server_port)
    debugf("[socket] WARNING! Incoming packet server port{%d} does NOT match "
           "captured one{%d}!",
           browse->server_port, server_port);

  if (*(uint32_t *)(&browse->server_ip[0]) &&
      memcmp(ipv4->source_address, browse->server_ip, 4) != 0) {
    debugf("[socket] Incoming packet (server) IP{%d.%d.%d.%d} does not match "
           "captured (server) IP{%d.%d.%d.%d}",
           ipv4->source_address[0], ipv4->source_address[1],
           ipv4->source_address[2], ipv4->source_address[3],
           browse->server_ip[0], browse->server_ip[1], browse->server_ip[2],
           browse->server_ip[3]);
    return false;
  }

  // spinlockAcquire(&browse->LOCK_PACKET);
  uint32_t usedSpace =
      (browse->recvBuffRecv <= browse->recvBuffSend)
          ? (browse->recvBuffSend - browse->recvBuffRecv)
          : (SOCK_RECV_BUFSIZE - (browse->recvBuffRecv - browse->recvBuffSend));
  uint32_t freeSpace = SOCK_RECV_BUFSIZE - usedSpace - 1;

  if (size > freeSpace) {
    // spinlockRelease(&browse->LOCK_PACKET);
    debugf("lost! size{%d} freeSpace{%d}\n", size, freeSpace);
    return false; // not enough space
  }

  uint8_t *body8 = (uint8_t *)body;
  for (int i = 0; i < size; i++) {
    browse->recvBuff[browse->recvBuffSend] = body8[i];
    browse->recvBuffSend = (browse->recvBuffSend + 1) % SOCK_RECV_BUFSIZE;
  }

  // spinlockRelease(&browse->LOCK_PACKET);
  return true;
}

uint32_t netSocketRecv(Socket *socket, uint8_t *buff, uint32_t size) {
  spinlockAcquire(&socket->LOCK_PACKET);

  uint32_t usedSpace =
      (socket->recvBuffRecv <= socket->recvBuffSend)
          ? (socket->recvBuffSend - socket->recvBuffRecv)
          : (SOCK_RECV_BUFSIZE - (socket->recvBuffRecv - socket->recvBuffSend));

  int bytesToRead = (size > usedSpace) ? usedSpace : size;

  for (int i = 0; i < bytesToRead; i++) {
    buff[i] = socket->recvBuff[socket->recvBuffRecv];
    socket->recvBuffRecv = (socket->recvBuffRecv + 1) % SOCK_RECV_BUFSIZE;
  }

  spinlockRelease(&socket->LOCK_PACKET);
  return bytesToRead;
}

void netSocketRecvCleanup(socketPacketHeader *packet) {
  // just in case
  // free(packet);
}
