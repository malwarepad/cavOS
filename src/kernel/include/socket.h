#include "nic_controller.h"
#include "types.h"

#ifndef SOCKET_H
#define SOCKET_H

Socket *netSocketConnect(NIC *nic, SOCKET_PROT protocol,
                         uint8_t *destination_ip, uint16_t source_port,
                         uint16_t destination_port);
bool netSocketPass(NIC *nic, SOCKET_PROT protocol, void *body, uint32_t size);
uint32_t netSocketRecv(Socket *socket, uint8_t *buff, uint32_t size);
void     netSocketRecvCleanup(socketPacketHeader *packet);

#endif
