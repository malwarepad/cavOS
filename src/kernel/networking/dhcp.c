#include <arp.h>
#include <checksum.h>
#include <dhcp.h>
#include <liballoc.h>
#include <system.h>
#include <udp.h>
#include <util.h>

// Dynamic Host Configuration Protocol, according to:
// https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol
// Copyright (C) 2024 Panagiotis

uint8_t dhcpBroadcastMAC[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t dhcpBroadcastIp[] = {0xff, 0xff, 0xff, 0xff};

void netDHCPapproveOptions(NIC *nic) {
  uint8_t    *body = (uint8_t *)malloc(sizeof(dhcpHeader) + 32);
  dhcpHeader *header = (dhcpHeader *)body;
  memset(header, 0, sizeof(dhcpHeader));

  header->opcode = DHCP_SEND;
  header->htype = DHCP_TYPE_ETH;
  header->hlen = 0x06;
  header->hops = 0x00;

  header->xid = switch_endian_32(nic->dhcpTransactionID);

  header->secs = switch_endian_16(0x0);
  header->flags = switch_endian_16(0x0);

  memset(header->client_address, 0, 4);
  memcpy(header->your_ip, nic->ip, 4);
  memcpy(header->server_ip, nic->serverIp, 4);
  memset(header->gateway_ip, 0, 4);

  memcpy(header->client_mac, nic->MAC, 6);

  header->signature = switch_endian_32(DHCP_SIGNATURE);

  uint32_t extras = 0;

  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_MESSAGE_TYPE;
  body[sizeof(dhcpHeader) + (extras++)] = 1;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_REQUEST;

  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_REQUESTED_IP;
  body[sizeof(dhcpHeader) + (extras++)] = 4;
  body[sizeof(dhcpHeader) + (extras++)] = header->your_ip[0];
  body[sizeof(dhcpHeader) + (extras++)] = header->your_ip[1];
  body[sizeof(dhcpHeader) + (extras++)] = header->your_ip[2];
  body[sizeof(dhcpHeader) + (extras++)] = header->your_ip[3];

  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_SERVER_ID;
  body[sizeof(dhcpHeader) + (extras++)] = 4;
  body[sizeof(dhcpHeader) + (extras++)] = header->server_ip[0];
  body[sizeof(dhcpHeader) + (extras++)] = header->server_ip[1];
  body[sizeof(dhcpHeader) + (extras++)] = header->server_ip[2];
  body[sizeof(dhcpHeader) + (extras++)] = header->server_ip[3];

  body[sizeof(dhcpHeader) + (extras++)] = 0xff;

  netUdpSend(nic, dhcpBroadcastMAC, dhcpBroadcastIp, body,
             sizeof(dhcpHeader) + extras, 68, 67);

  free(body);
}

void netDHCPreceive(NIC *nic, void *body, uint32_t size) {
  udpHeader *udp =
      (uint32_t)body + sizeof(netPacketHeader) + sizeof(IPv4header);
  dhcpHeader *dhcp = (uint32_t)udp + sizeof(udpHeader);
  uint8_t    *dhcpOptions = (uint32_t)dhcp + sizeof(dhcpHeader);

  if (switch_endian_32(dhcp->xid) != nic->dhcpTransactionID)
    return;

  if (dhcp->opcode != DHCP_RECEIVE) {
    debugf("[networking::dhcp] Not-a-server! opcode{0x%02X}\n", dhcp->opcode);
    return;
  }

  // the comment on dhcp.h explains this rather odd way of fetching options
  bool optionFetch = true;

  // scan n1; identify message/option type so we can proceed
  uint8_t dhcpMessageType = 0;
  while (optionFetch) { // scan n1
    switch (dhcpOptions[0]) {
    case DHCP_OPTION_MESSAGE_TYPE:
      dhcpMessageType = dhcpOptions[2];
      break;
    case DHCP_OPTION_END:
      optionFetch = false;
      break;
    default:
      break;
    }
    if (optionFetch)
      dhcpOptions = (uint32_t)dhcpOptions + 1 + 1 +
                    dhcpOptions[1]; // type(1) + size(1) + rest(?)
  }

  switch (dhcpMessageType) {
  case DHCP_OFFER:
    memcpy(nic->ip, dhcp->your_ip, 4); // get our ip

    // scan n2; scan all fields now, knowing it's a DHCP "OFFER"
    optionFetch = true;
    dhcpOptions = (uint32_t)dhcp + sizeof(dhcpHeader);
    while (optionFetch) { // scan n2
      switch (dhcpOptions[0]) {
      case DHCP_OPTION_MESSAGE_TYPE:
        if (dhcpOptions[2] != dhcpMessageType)
          debugf("[networking::dhcp] Something incredibely fucked up is going "
                 "on: n2_scan_option{%02X} dhcpMessageType{%02X}\n",
                 dhcpOptions[2], dhcpMessageType);
        break;
      case DHCP_OPTION_ROUTER:
        memcpy(nic->serverIp, (uint32_t)dhcpOptions + 2, 4);
        break;
      case DHCP_OPTION_DNS_SERVER:
        memcpy(nic->dnsIp, (uint32_t)dhcpOptions + 2, 4);
        break;
      case DHCP_OPTION_SUBNET_MASK:
        memcpy(nic->subnetMask, (uint32_t)dhcpOptions + 2, 4);
        break;
      case DHCP_OPTION_LEASE_TIME:
        break;
      case DHCP_OPTION_END:
        optionFetch = false;
        break;
      }
      if (optionFetch)
        dhcpOptions = (uint32_t)dhcpOptions + 1 + 1 +
                      dhcpOptions[1]; // type(1) + size(1) + rest(?)
    }

    // if options have no router ip, get it from header (if it has it)
    if (!(*((uint32_t *)nic->serverIp)) && *((uint32_t *)dhcp->server_ip))
      memcpy(nic->serverIp, dhcp->server_ip, 4);

    netDHCPapproveOptions(nic);
    break;
  case DHCP_ACK:
    // done...
    netUdpRemove(nic, 68);
    break;
  default:
    debugf("[networking::dhcp] Odd DHCP message type! %d\n", dhcpMessageType);
    break;
  }
}

void netDHCPinit(NIC *nic) {
  uint8_t    *body = (uint8_t *)malloc(sizeof(dhcpHeader) + 32);
  dhcpHeader *header = (dhcpHeader *)body;
  memset(header, 0, sizeof(dhcpHeader));

  header->opcode = DHCP_SEND;
  header->htype = DHCP_TYPE_ETH;
  header->hlen = 0x06;
  header->hops = 0x00;

  header->xid = switch_endian_32(nic->dhcpTransactionID);

  header->secs = switch_endian_16(0x0);
  header->flags = switch_endian_16(0x0);

  memset(header->client_address, 0, 4);
  memset(header->your_ip, 0, 4);
  memset(header->server_ip, 0, 4);
  memset(header->gateway_ip, 0, 4);

  memcpy(header->client_mac, nic->MAC, 6);

  header->signature = switch_endian_32(DHCP_SIGNATURE);

  uint32_t extras = 0;

  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_MESSAGE_TYPE;
  body[sizeof(dhcpHeader) + (extras++)] = 1;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_DISCOVERY;

  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_REQUEST_PARAMETERS_LIST;
  body[sizeof(dhcpHeader) + (extras++)] = 4;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_ROUTER;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_DNS_SERVER;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_SUBNET_MASK;
  body[sizeof(dhcpHeader) + (extras++)] = DHCP_OPTION_LEASE_TIME;

  body[sizeof(dhcpHeader) + (extras++)] = 0xff;

  netUdpRegister(nic, 68, netDHCPreceive);
  netUdpSend(nic, dhcpBroadcastMAC, dhcpBroadcastIp, body,
             sizeof(dhcpHeader) + extras, 68, 67);

  free(body);
}
