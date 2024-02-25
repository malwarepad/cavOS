#include <checksum.h>
#include <ipv4.h>
#include <liballoc.h>
#include <system.h>
#include <udp.h>
#include <util.h>

void netUdpSend(NIC *nic, uint8_t *destination_mac, uint8_t *destination_ip,
                void *data, uint32_t data_size, uint16_t source_port,
                uint16_t destination_port) {
  uint32_t   finalSize = sizeof(udpHeader) + data_size;
  uint8_t   *final = malloc(finalSize);
  udpHeader *header = (udpHeader *) final;

  memset(header, 0, sizeof(udpHeader));

  header->source_port = switch_endian_16(source_port);
  header->destination_port = switch_endian_16(destination_port);

  header->length = switch_endian_16(finalSize);
  header->checksum = 0; // todo (maybe)

  // calculate checksum before request's finalized
  // header->checksum = checksum(header, sizeof(udpHeader));

  memcpy((uint32_t) final + sizeof(udpHeader), data, data_size);

  netIPv4Send(nic, destination_mac, destination_ip, final, finalSize,
              UDP_PROTOCOL);

  free(final);
}

udpHeader *netUdpFindByPort(NIC *nic, uint16_t port) {
  udpHandler *curr = nic->firstUdpHandler;
  while (curr) {
    if (curr->port == port)
      break;
    curr = curr->next;
  }
  return curr;
}

udpHandler *netUdpRegister(NIC *nic, uint16_t port, void *targetHandler) {
  if (netUdpFindByPort(nic, port)) {
    debugf("[networking::udp] Tried to register another connection on "
           "port{%d}, aborting!\n",
           port);
    return 0;
  }

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
    if (curr->next && (udpHandler *)(curr->next)->port == port)
      break;
    curr = curr->next;
  }
  if (nic->firstUdpHandler && nic->firstUdpHandler->port == port) {
    udpHandler *target = nic->firstUdpHandler;
    nic->firstUdpHandler = target->next;
    free(target);
    return true;
  } else if (!curr)
    return false;

  udpHandler *target = curr->next;
  curr->next = target->next; // remove reference
  free(target);              // free remaining memory

  return true;
}

bool netUdpVerify(NIC *nic, udpHeader *header, uint32_t size) {
  uint32_t actualSize = size - sizeof(netPacketHeader) - sizeof(IPv4header);
  if (size > nic->mintu && switch_endian_16(header->length) != actualSize) {
    debugf("[networking::udp] Bad length, ignoring! header{%d} != actual{%d}\n",
           switch_endian_16(header->length), actualSize);
    return false;
  }

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

  if (!netUdpVerify(nic, header, size))
    return;

  // todo: create a global socket-like userspace & kernelspace interface to
  // avoid overloading interrupt handlers and prepare for userland triggered
  // systemcalls
  browse->handler(nic, body, size, browse);
}
