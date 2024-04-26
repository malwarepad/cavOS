#include "types.h"

#ifndef CHECKSUM_H
#define CHECKSUM_H

uint16_t checksum(void *addr, int count);
uint16_t tcpChecksum(void *addr, uint32_t count, uint8_t *source_address,
                     uint8_t *destination_ip);
bool     isLocalIPv4(const uint8_t *ip);
void     ipPrompt(uint8_t *out);

#endif
