#include "pci.h"
#include "types.h"

#ifndef NIC_CONTROLLER_H
#define NIC_CONTROLLER_H

/* NICs */

typedef enum NIC_TYPE { NE2000, RTL8139, RTL8169 } NIC_TYPE;

typedef struct NIC NIC;

// why are there ipv4 stuff here???!
typedef struct IPv4fragmentedPacketRaw IPv4fragmentedPacketRaw;
struct IPv4fragmentedPacketRaw {
  IPv4fragmentedPacketRaw *next;

  uint16_t index;
  uint16_t size;
  uint8_t *buffer;
};
typedef struct IPv4fragmentedPacket IPv4fragmentedPacket;
struct IPv4fragmentedPacket {
  IPv4fragmentedPacket *next;

  // todo: create a system for discarding those...
  uint8_t                  source_address[4];
  uint16_t                 id;
  IPv4fragmentedPacketRaw *firstPacket;
  IPv4fragmentedPacketRaw *lastPacket;

  uint32_t curr;
  uint32_t max;
};

// why are there arp stuff here???!
#define ARP_TABLE_LEN 512
typedef struct arpTableEntry {
  uint8_t ip[4];
  uint8_t mac[6];
} arpTableEntry;

// why are there udp stuff here???!
typedef void (*NetHandlerFunction)(NIC *nic, void *body, uint32_t size,
                                   void *extras);
typedef struct udpHandler udpHandler;
struct udpHandler {
  udpHandler *next;

  uint16_t           port; // dest
  NetHandlerFunction handler;
};

// why are there tcp stuff here???!
typedef struct tcpPacketHeader tcpPacketHeader;
struct tcpPacketHeader {
  tcpPacketHeader *next;

  uint32_t size;
};
typedef struct tcpConnection tcpConnection;
struct tcpConnection {
  tcpConnection *next;

  bool     open;
  bool     closing;
  uint16_t client_port; // dest
  uint16_t server_port;

  uint8_t server_ip[4];
  // NetHandlerFunction handler; not a good idea, it bypasses other interrutps
  // from going through!

  uint32_t client_seq_number;
  uint32_t client_ack_number;

  tcpPacketHeader *firstPendingPacket;
};

struct NIC {
  NIC_TYPE type;
  uint16_t mtu;
  uint8_t  mintu;
  void    *infoLocation;
  uint8_t  MAC[6];
  uint8_t  ip[4];
  uint8_t  serverIp[4];
  uint8_t  dnsIp[4];
  uint8_t  subnetMask[4];
  uint8_t  irq;

  // IPv4
  IPv4fragmentedPacket *firstFragmentedPacket;

  // ARP
  arpTableEntry arpTable[ARP_TABLE_LEN];
  uint32_t      arpTableCurr;

  // UDP
  udpHandler *firstUdpHandler;

  // TCP
  tcpConnection *firstTcpConnection;

  // DHCP
  bool     dhcpUdpRegistered;
  uint32_t dhcpTransactionID;
};
#define defaultIP ((uint8_t[]){0, 0, 0, 0})
#define macBroadcast ((uint8_t[]){255, 255, 255, 255, 255, 255})
#define macZero ((uint8_t[]){0, 0, 0, 0, 0, 0})

NIC *selectedNIC;

void initiateNetworking();

// returns UNINITIALIZED!! NIC struct
void initiateNIC(PCIdevice *device);
NIC *createNewNIC(PCI *pci);

/* Packets */
typedef struct netPacketHeader {
  uint8_t  destination_mac[6];
  uint8_t  source_mac[6];
  uint16_t ethertype;
} __attribute__((packed)) netPacketHeader;

typedef struct netPacket {
  netPacketHeader header;
  void           *data;
} netPacket;

/* Ethertypes/Protocols */
enum NET_ETHERTYPES {
  NET_ETHERTYPE_ARP = 0x0806,
  NET_ETHERTYPE_IPV4 = 0x0800,
  NET_ETHERTYPE_IPV6 = 0x86DD
};

void sendPacket(NIC *nic, uint8_t *destination_mac, void *data, uint32_t size,
                uint16_t protocol);
void handlePacket(NIC *nic, void *packet, uint32_t size);

#endif
