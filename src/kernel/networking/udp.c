#include <checksum.h>
#include <ipv4.h>
#include <malloc.h>
#include <socket.h>
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

  memcpy((size_t) final + sizeof(udpHeader), data, data_size);

  netIPv4Send(nic, destination_mac, destination_ip, final, finalSize,
              UDP_PROTOCOL);

  free(final);
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
      (size_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);

  if (!netUdpVerify(nic, header, size))
    return;

  netSocketPass(nic, SOCKET_PROT_UDP, body, size);
}
