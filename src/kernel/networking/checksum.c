#include <checksum.h>
#include <kb.h>
#include <malloc.h>
#include <system.h>
#include <tcp.h>
#include <udp.h>
#include <util.h>

// Compute the internet checksum
// (shamelessly stolen from https://tools.ietf.org/html/rfc1071)
// Copyright (C) 2024 Panagiotis

uint16_t checksum(void *addr, int count) {
  register uint32_t sum = 0;
  uint16_t         *ptr = addr;

  while (count > 1) {
    /*  This is the inner loop */
    sum += *ptr++;
    count -= 2;
  }

  /*  Add left-over byte, if any */
  if (count > 0)
    sum += *(uint8_t *)ptr;

  /*  Fold 32-bit sum to 16 bits */
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

typedef struct TCPchecksumIPv4 {
  uint8_t  source_address[4];
  uint8_t  destination_address[4];
  uint8_t  zeros;
  uint8_t  protocol;
  uint16_t tcp_length;
} TCPchecksumIPv4;

uint16_t tcpChecksum(void *addr, uint32_t count, uint8_t *source_address,
                     uint8_t *destination_ip) {
  uint8_t *fakesumBody = (uint8_t *)malloc(sizeof(TCPchecksumIPv4) + count);
  TCPchecksumIPv4 *fakesumHeader = (TCPchecksumIPv4 *)fakesumBody;
  memset(fakesumHeader, 0, sizeof(TCPchecksumIPv4));

  memcpy(fakesumHeader->source_address, source_address, 4);
  memcpy(fakesumHeader->destination_address, destination_ip, 4);
  fakesumHeader->tcp_length = switch_endian_16(count);
  fakesumHeader->protocol = TCP_PROTOCOL;

  memcpy((size_t)fakesumBody + sizeof(TCPchecksumIPv4), addr, count);

  uint16_t res = checksum(fakesumBody, sizeof(TCPchecksumIPv4) + count);
  free(fakesumBody);

  return res;
}

// I don't feel like making a new file for this, checksum.c she goes...
bool isLocalIPv4(uint8_t *ip) {
  uint32_t ipAddress = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
                       ((uint32_t)ip[2] << 8) | (uint32_t)ip[3];
  return (ipAddress & 0xFF000000) == 0x0A000000 || // 10.0.0.0/8
         (ipAddress & 0xFFF00000) == 0xAC100000 || // 172.16.0.0/12
         (ipAddress & 0xFFFF0000) == 0xC0A80000;   // 192.168.0.0/16
}

void ipPrompt(uint8_t *out) {
  char *curr = (char *)malloc(8);

  for (uint8_t i = 0; i < 4; i++) {
    memset(curr, 0, 8);
    readStr(curr);
    if (i < 3)
      printf(".");
    out[i] = atoi(curr);
  }

  free(curr);
}
