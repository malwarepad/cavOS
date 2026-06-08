#include <ipv4.h>
#include <nic_controller.h>
#include <udp.h>

// Layer3: Internet Protocol (v4)
// Copyright (C) 2025 Panagiotis

extern Task *netHelperTask;

uint16_t netIPv4Checksum(void *addr, int count) {
  register uint32_t sum = 0;
  uint16_t         *ptr = addr;

  while (count > 1) {
    // inner loop
    sum += *ptr++;
    count -= 2;
  }

  // add left-over byte, if any
  if (count > 0)
    sum += *(uint8_t *)ptr;

  // fold 32-bit sum to 16 bits
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

void netIPv4Handle(void *_nic, void *packet, uint32_t size) {
  IPv4header *ipv4 = NET_IPv4(packet);

  NIC *nic = _nic;
  if (size < NET_IPv4_CARRY + sizeof(IPv4header)) {
    debugf("[net::ipv4] Drop: Too small\n");
    return;
  }

  if (ipv4->version != 4) {
    debugf("[net::ipv4] Drop: IP version is not equal to 4\n");
    return;
  }

  if (switch_endian_16(ipv4->flags) & IPV4_FLAGS_MORE_FRAGMENTS ||
      IPV4_FRAGMENT_OFFSET(switch_endian_16(ipv4->flags)) != 0) {
    debugf("[net::ipv4] Drop: Fragmentation is not supported\n");
    return;
  }

  if (switch_endian_16(ipv4->length) > size - NET_IPv4_CARRY) {
    debugf("[net::ipv4] Drop: Invalid IPv4 length advertised\n");
    return;
  }

  uint32_t headerLen = ipv4->ihl * sizeof(uint32_t);
  if (headerLen < sizeof(IPv4header)) {
    debugf("[net::ipv4] Drop: IHL is smaller than the base IPv4 header\n");
    return;
  }

  // todo: check for headerLen overflows (keep padding in mind!)
  // same in tcp

  if (memcmp(ipv4->destAddress, nic->ip, IPv4_BYTE_SIZE) != 0 &&
      memcmp(ipv4->destAddress, addressBroadcast, IPv4_BYTE_SIZE) != 0 &&
      memcmp(ipv4->destAddress, addressNull, IPv4_BYTE_SIZE) != 0) {
    debugf("[net::ipv4] Drop: Packet destination isn't us nor broadcast\n");
    return;
  }

  if (ipv4->ttl == 1) {
    debugf("[net::ipv4] Drop: TTL would reach 0\n");
    return;
  }

  // todo: look into lwip's 3rd algorithm for this, looks better than stock RFC
  if (netIPv4Checksum(ipv4, ipv4->ihl * sizeof(uint32_t)) != 0) {
    debugf("[net::ipv4] Drop: Checksum validation failed\n");
    return;
  }

  // ipv4 is the first layer where padding can be introduced
  // a lot of stuff on our stack depends on an accurate packet size, hence this
  uint32_t padding = size - switch_endian_16(ipv4->length) - NET_IPv4_CARRY;
  size -= padding;

  switch (ipv4->protocol) {
  case IPV4_PROTOCOL_TCP:
    // a
    break;
  case IPV4_PROTOCOL_UDP:
    netUdpHandle(_nic, packet, size);
    break;
  case IPV4_PROTOCOL_ICMP:
    // a
    break;
  default:
    debugf("[net::ipv4] Drop: Unhandled protocol\n");
    break;
  }
}

bool netIPv4NeedsRouting(uint8_t *localIp, uint8_t *destIp,
                         uint8_t *subnetMask) {
  uint32_t localIpCasted = *(uint32_t *)(localIp);
  uint32_t destIpCasted = *(uint32_t *)(destIp);
  uint32_t subnetMaskCasted = *(uint32_t *)(subnetMask);

  return (destIpCasted & subnetMaskCasted) !=
         (localIpCasted & subnetMaskCasted);
}

void netIPv4InitBuffer(void *buffer, uint32_t size) {
  assert(size >= NET_IPv4_CARRY + sizeof(IPv4header));
  IPv4header *ipv4 = NET_IPv4(buffer);
  ipv4->ihl = sizeof(IPv4header) / sizeof(uint32_t);
}

void netIPv4Send(void *_nic, void *packet, uint32_t size, uint8_t protocol,
                 uint8_t *destinationIp) {
  assert(size >= NET_IPv4_CARRY + sizeof(IPv4header));
  IPv4header *ipv4 = NET_IPv4(packet);
  assert(ipv4->ihl == sizeof(IPv4header) / sizeof(uint32_t));

  NIC *nic = _nic;
  memset(ipv4, 0, sizeof(IPv4header));
  ipv4->ihl = sizeof(IPv4header) / sizeof(uint32_t);
  ipv4->version = 4;
  ipv4->tos = 0;

  // keep in mind that we aren't using any options
  ipv4->length = switch_endian_16(size - NET_IPv4_CARRY);
  ipv4->id = switch_endian_16(rand());
  ipv4->flags = 0;
  ipv4->ttl = 255;
  ipv4->protocol = protocol;
  ipv4->checksum = 0; // will be filled in later
  memcpy(ipv4->srcAddress, nic->ip, IPv4_BYTE_SIZE);
  memcpy(ipv4->destAddress, destinationIp, IPv4_BYTE_SIZE);

  // calculate the ipv4 checksum (mandatory)
  ipv4->checksum = netIPv4Checksum(ipv4, ipv4->ihl * sizeof(uint32_t));

  uint8_t routeIp[IPv4_BYTE_SIZE];
  if (netIPv4NeedsRouting(nic->ip, destinationIp, nic->subnetMask))
    memcpy(routeIp, nic->serverIp, IPv4_BYTE_SIZE);
  else
    memcpy(routeIp, destinationIp, IPv4_BYTE_SIZE);

  uint8_t destinationMac[MAC_BYTE_SIZE] = {0};
  if (currentTask != netHelperTask) {
    while (!netArpTranslate(_nic, routeIp, destinationMac))
      handControl(); // todo: some kind of timeout (except the retry one)
  } else {
    if (!netArpTranslate(_nic, routeIp, destinationMac))
      assert(false); // todo (although said code path is not yet needed)
  }
  netEthSend(_nic, packet, size, destinationMac, NET_ETHERTYPE_IPV4);
}
