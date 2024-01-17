#include <checksum.h>
#include <ipv4.h>
#include <liballoc.h>
#include <system.h>
#include <udp.h>
#include <util.h>

void netUdpSend(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                void *data, uint32_t data_size, uint16_t source_port,
                uint16_t destination_port) {
  uint8_t   *final = malloc(sizeof(udpHeader) + data_size);
  udpHeader *header = (udpHeader *) final;

  memset(header, 0, sizeof(udpHeader));

  header->source_port = switch_endian_16(source_port);
  header->destination_port = switch_endian_16(destination_port);

  header->length = switch_endian_16(sizeof(udpHeader) + data_size);
  header->checksum = 0; // todo (maybe)

  // calculate checksum before request's finalized
  // header->checksum = checksum(header, sizeof(udpHeader));

  memcpy((uint32_t) final + sizeof(udpHeader), data, data_size);

  netIPv4Send(nic, destination_mac, destination_ip, final,
              sizeof(udpHeader) + data_size, UDP_PROTOCOL);

  free(final);
}

udpHandler *netUdpRegister(NIC *nic, uint16_t port, void *targetHandler) {
  netUdpRemove(nic, port); // ensure no old ones left

  udpHandler *handler = (udpHandler *)malloc(sizeof(udpHandler));
  memset(handler, 0, sizeof(udpHandler));
  udpHandler *curr = nic->firstUdpHandler;
  while (1) {
    if (curr == 0) {
      // means this is our first one
      nic->firstUdpHandler = handler;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = handler;
      break;
    }
    curr = curr->next; // cycle
  }

  handler->handler = targetHandler;
  handler->port = port;
  handler->next = 0; // null ptr
  return handler;
}

bool netUdpRemove(NIC *nic, uint16_t port) {
  udpHandler *curr = nic->firstUdpHandler;
  while (curr) {
    if (curr->next == port)
      break;
    curr = curr->next;
  }
  if (!curr)
    return false;

  udpHandler *target = curr->next;
  curr->next = target->next; // remove reference
  free(target);              // free remaining memory

  return true;
}

void netUdpReceive(NIC *nic, void *body, uint32_t size) {
  udpHeader *header =
      (uint32_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);

  udpHandler *browse = nic->firstUdpHandler;
  while (browse) {
    if (browse->port == switch_endian_16(header->destination_port))
      break;
    browse = browse->next;
  }
  if (!browse || !browse->handler)
    return;

  browse->handler(nic, body, size);
}
