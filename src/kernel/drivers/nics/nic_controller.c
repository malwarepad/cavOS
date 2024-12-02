#include <arp.h>
#include <dhcp.h>
#include <ipv4.h>
#include <kernel_helper.h>
#include <linked_list.h>
#include <malloc.h>
#include <ne2k.h>
#include <nic_controller.h>
#include <rtl8139.h>
#include <rtl8169.h>
#include <system.h>
#include <util.h>

#include <timer.h>

#include <lwip/api.h>
#include <lwip/dhcp.h>
#include <lwip/etharp.h>
#include <lwip/ip_addr.h>
#include <lwip/tcpip.h>

// Manager for all connected network interfaces
// Copyright (C) 2024 Panagiotis

err_t lwipDummyInit(struct netif *netif) { return ERR_OK; }

void initiateNetworking() {
  // start off with no first NIC and no selected one
  // rest on device-specific initialization
  selectedNIC = 0;
  debugf("[networking] Ready to scan for NICs..\n");
}

err_t lwipOutput(struct netif *netif, struct pbuf *p) {
  uint8_t     *complete = malloc(p->tot_len);
  struct pbuf *browse = p;
  uint32_t     cnt = 0;

  while (browse) {
    memcpy(&complete[cnt], browse->payload, browse->len);
    cnt += browse->len;
    browse = browse->next;
  }

  if (cnt != p->tot_len) {
    debugf("[networking::lwipOut] Something is seriously wrong! tot_len{%d} "
           "cnt{%d}\n",
           p->tot_len, cnt);
    panic();
  }

  PCI *pci = firstPCI;
  while (pci) {
    NIC *nic = (NIC *)pci->extra;
    if (pci->category == PCI_DRIVER_CATEGORY_NIC && &nic->lwip == netif)
      break;
    pci = pci->next;
  }

  if (!pci) {
    debugf("[nics] Coudln't find netif to pass!\n");
    panic();
  }

  NIC *nic = (NIC *)pci->extra;
  sendPacketRaw(nic, complete, p->tot_len);
  free(complete);
  return ERR_OK;
}

void lwipInitInThread(void *nicPre) {
  NIC *nic = (NIC *)nicPre;
  // struct ethernetif *ethernetif;
  struct ip4_addr netmask;

  struct netif *this_netif = &nic->lwip;

  this_netif->state = NULL;
  this_netif->name[0] = 65;
  this_netif->name[1] = 66;
  this_netif->next = NULL;

  IP4_ADDR(&netmask, 255, 255, 255, 0);
  netif_add(this_netif, NULL, &netmask, NULL, NULL, lwipDummyInit,
            tcpip_input); // ethernetif_init_low

  this_netif->output = etharp_output;
  this_netif->linkoutput = lwipOutput;
  netif_set_default(this_netif);
  this_netif->hwaddr_len = ETHARP_HWADDR_LEN;
  this_netif->hwaddr[0] = nic->MAC[0]; // or whatever u like
  this_netif->hwaddr[1] = nic->MAC[1];
  this_netif->hwaddr[2] = nic->MAC[2];
  this_netif->hwaddr[3] = nic->MAC[3];
  this_netif->hwaddr[4] = nic->MAC[4];
  this_netif->hwaddr[5] = nic->MAC[5];
  this_netif->mtu = nic->mtu;
  this_netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                      NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP;
  netif_set_up(this_netif);
  err_t out = dhcp_start(this_netif);
  if (out != ERR_OK) {
    debugf("[nic::lwip] Couldn't start dhcp!\n");
    panic();
  }
  sys_check_timeouts();
  dhcp_supplied_address(this_netif);
}

void initiateNIC(PCIdevice *device) {
  if (initiateNe2000(device) || initiateRTL8139(device) ||
      initiateRTL8169(device)) {
    // selectedNIC = newly created NIC structure
    tcpip_init(lwipInitInThread, selectedNIC);
  }
}

// returns UNINITIALIZED!! NIC struct
NIC *createNewNIC(PCI *pci) {
  NIC *nic = malloc(sizeof(NIC));
  memset(nic, 0, sizeof(NIC));

  nic->dhcpTransactionID = rand();
  nic->mtu = 1500;

  pci->extra = nic;
  selectedNIC = nic;
  return nic;
}

void sendPacket(NIC *nic, uint8_t *destination_mac, void *data, uint32_t size,
                uint16_t protocol) {
  if ((size + sizeof(netPacketHeader)) > nic->mtu) {
    debugf("[nics] FATAL! Packet size{%d} is larger than said NIC's MTU{%d}\n",
           sizeof(netPacketHeader) + size, nic->mtu);
    return;
  }
  netPacketHeader *packet = malloc(sizeof(netPacketHeader) + size);
  void            *packetData = (void *)packet + sizeof(netPacketHeader);

  memcpy(packet->source_mac, nic->MAC, 6);
  memcpy(packet->destination_mac, destination_mac, 6);
  packet->ethertype = switch_endian_16(protocol);

  memcpy(packetData, data, size);

  switch (nic->type) {
  case NE2000:
    sendNe2000(nic, packet, sizeof(netPacketHeader) + size);
    break;
  case RTL8139:
    sendRTL8139(nic, packet, sizeof(netPacketHeader) + size);
    break;
  case RTL8169:
    sendRTL8169(nic, packet, sizeof(netPacketHeader) + size);
    break;
  }

  free(packet);
}

void sendPacketRaw(NIC *nic, void *data, uint32_t size) {
  if (size > nic->mtu) {
    debugf("[nics] FATAL! Packet size{%d} is larger than said NIC's MTU{%d}\n",
           size, nic->mtu);
    return;
  }

  switch (nic->type) {
  case NE2000:
    sendNe2000(nic, data, size);
    break;
  case RTL8139:
    sendRTL8139(nic, data, size);
    break;
  case RTL8169:
    sendRTL8169(nic, data, size);
    break;
  }
}

void handlePacket(NIC *nic, void *packet, uint32_t size) {
  struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_RAM);
  uint8_t     *targ = (uint8_t *)p->payload;
  for (int i = 0; i < size; i++)
    targ[i] = ((uint8_t *)packet)[i];
  nic->lwip.input(p, &nic->lwip);
}

// outside stuff

QueuePacket netQueue[QUEUE_MAX];
int         netQueueRead = 0;
int         netQueueWrite = 0;

void netQueueAdd(NIC *nic, uint8_t *packet, uint16_t packetLength) {
  if ((netQueueWrite + 1) % QUEUE_MAX == netQueueRead) {
    debugf("[netqueue] New %d length packet dropped!\n", packetLength);
    return;
  }

  QueuePacket *item = &netQueue[netQueueWrite];
  item->nic = nic;
  memcpy(item->buff, packet, packetLength);
  item->packetLength = packetLength;

  netQueueWrite = (netQueueWrite + 1) % QUEUE_MAX;

  // direct the task
  // netHelperTask->state = TASK_STATE_READY;
}
