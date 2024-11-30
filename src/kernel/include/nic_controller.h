#include "pci.h"
#include "system.h"
#include "types.h"

#include <lwip/netif.h>

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

// why is there socket stuff here???!
typedef enum SOCKET_PROT {
  SOCKET_PROT_NULL = 0,
  SOCKET_PROT_TCP,
  SOCKET_PROT_UDP
} SOCKET_PROT;

typedef struct socketPacketHeader socketPacketHeader;
struct socketPacketHeader {
  socketPacketHeader *next;
  uint32_t            size;
};

#define SOCK_RECV_BUFSIZE 212992

typedef struct Socket Socket;
struct Socket {
  Socket *next;

  SOCKET_PROT protocol;
  void       *protocolSpecific;

  uint8_t  server_ip[4];
  uint8_t  server_mac[6];
  uint16_t client_port; // dest
  uint16_t server_port;

  uint32_t recvBuffRecv;
  uint32_t recvBuffSend;
  uint8_t  recvBuff[SOCK_RECV_BUFSIZE];
  Spinlock LOCK_PACKET;
};

struct NIC {
  struct netif lwip;

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

  // DHCP
  Socket  *dhcpUdpRegistered;
  uint32_t dhcpTransactionID;

  // Sockets
  Socket *firstSocket;
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
} netPacketHeader;

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
void sendPacketRaw(NIC *nic, void *data, uint32_t size);
void handlePacket(NIC *nic, void *packet, uint32_t size);

// outside stuff

#define PACKET_MAX 1600
#define QUEUE_MAX 128
typedef struct QueuePacket {
  bool exists;

  NIC *nic;

  uint8_t  buff[PACKET_MAX];
  uint16_t packetLength;
} QueuePacket;

QueuePacket netQueue[QUEUE_MAX];
int         netQueueRead;
int         netQueueWrite;

void netQueueAdd(NIC *nic, uint8_t *packet, uint16_t packetLength);

#endif
