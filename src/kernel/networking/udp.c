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
