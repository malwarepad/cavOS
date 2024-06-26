#include "nic_controller.h"
#include "pci.h"
#include "types.h"

#ifndef RTL8169_H
#define RTL8169_H

#define RTL8169_DESCRIPTORS 1024
#define RTL8169_RX_DESCRIPTORS RTL8169_DESCRIPTORS
#define RTL8169_TX_DESCRIPTORS RTL8169_DESCRIPTORS

#define RTL8169_OWN 0x80000000
#define RTL8169_EOR 0x40000000

#define RTL8169_RECV 0x01
#define RTL8169_SENT 0x04
#define RTL8169_LINK_CHANGE 0x20

typedef struct rtl8169_descriptor {
  uint32_t command;  /* command/status uint32_t */
  uint32_t vlan;     /* currently unused */
  uint32_t low_buf;  /* low 32-bits of physical buffer address */
  uint32_t high_buf; /* high 32-bits of physical buffer address */
} rtl8169_descriptor;

typedef struct rtl8169_interface {
  uint16_t            iobase;
  rtl8169_descriptor *RxDescriptors; /* 1MB Base Address of Rx Descriptors */
  rtl8169_descriptor *TxDescriptors; /* 2MB Base Address of Tx Descriptors */
  bool                txSent;
} rtl8169_interface;

bool initiateRTL8169(PCIdevice *device);
void sendRTL8169(NIC *nic, void *packet, uint32_t packetSize);
void receiveRTL8169(NIC *nic);

#endif