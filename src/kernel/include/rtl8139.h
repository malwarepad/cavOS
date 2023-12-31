#include "nic_controller.h"
#include "pci.h"
#include "types.h"

#ifndef RTL8139_H
#define RTL8139_H

enum RTL8139Status {
  RTL8139_STATUS_TOK = (1 << 2),
  RTL8139_STATUS_ROK = (1 << 0)
};

enum RTL8139Registers {
  RTL8139_REG_MAC0_5 = 0x00,
  RTL8139_REG_MAC5_6 = 0x04,
  RTL8139_REG_MAR0_7 = 0x08,
  RTL8139_REG_RBSTART = 0x30,
  RTL8139_REG_CMD = 0x37,
  RTL8139_REG_IMR = 0x3C,
  RTL8139_REG_ISR = 0x3E,

  RTL8139_REG_POWERUP = 0x52
};

typedef struct rtl8139_interface {
  uint16_t iobase;
  uint8_t  tx_curr;
  void    *rx_buff_virtual; // physical can be computed if needed
} rtl8139_interface;

bool initiateRTL8139(PCIdevice *device);
void sendRTL8139(NIC *nic, void *packet, uint32_t packetSize);
void receiveRTL8139(NIC *nic);

#endif
