#include "pci.h"
#include "system.h"
#include "types.h"

#include <lwip/netif.h>

#ifndef NIC_CONTROLLER_H
#define NIC_CONTROLLER_H

/* NICs */

typedef enum NIC_TYPE { NE2000, RTL8139, RTL8169, E1000 } NIC_TYPE;

typedef struct NIC NIC;

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
