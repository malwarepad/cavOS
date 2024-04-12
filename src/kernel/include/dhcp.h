#include "ipv4.h"
#include "nic_controller.h"
#include "types.h"

#ifndef DHCP_H
#define DHCP_H

typedef struct dhcpHeader {
  uint8_t  opcode;
  uint8_t  htype;
  uint8_t  hlen;
  uint8_t  hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  uint8_t  client_address[4];
  uint8_t  your_ip[4];
  uint8_t  server_ip[4];
  uint8_t  gateway_ip[4];
  uint8_t  client_mac[6];
  uint8_t  reserved[10];
  uint8_t  server_name[64];
  uint8_t  boot_filename[128];
  uint32_t signature; // ppl call it cookie, magic cookie, whatever
} dhcpHeader;

/*
 * Option(s) layout! yeah....
 *  ______________________
 * | 0x35 |  0x01  | 0x02 |
 * | type | length | rest |
 *  ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
 * Type of 0x35 = 53, which is just a message/offer. Length of 0x01 = 1 (1
 * byte). The rest is the option of an offer.
 *
 * So, throughout code you will see this pattern of using [0] for the type, [1]
 * for the length. Note that the length itself is just the length of **the
 * rest** of said option. Full length of the option is 1 + 1 + length!
 */

typedef enum DHCP_OPTIONS {
  DHCP_OPTION_MESSAGE_TYPE = 53,
  DHCP_OPTION_REQUESTED_IP = 50,
  DHCP_OPTION_SERVER_ID = 54,
  DHCP_OPTION_REQUEST_PARAMETERS_LIST = 55,
  DHCP_OPTION_LEASE_TIME = 51,
  DHCP_OPTION_SUBNET_MASK = 1,
  DHCP_OPTION_ROUTER = 3,
  DHCP_OPTION_DNS_SERVER = 6,
  DHCP_OPTION_END = 255,
} DHCP_OPTIONS;

#define DHCP_TYPE_ETH 0x01

#define DHCP_SIGNATURE 0x63825363

#define DHCP_SEND 0x01
#define DHCP_RECEIVE 0x02

#define DHCP_TIMEOUT 2000

typedef enum DHCP_TYPES {
  DHCP_DISCOVERY = 0x01,
  DHCP_OFFER = 0x02,
  DHCP_REQUEST = 0x03,
  DHCP_ACK = 0x05,
} DHCP_TYPES;

void netDHCPinit(NIC *nic);

#endif
