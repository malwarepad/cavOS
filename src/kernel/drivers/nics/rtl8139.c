#include <isr.h>
#include <rtl8139.h>
#include <system.h>

#include <util.h>

#include <paging.h>
#include <pmm.h>
#include <vmm.h>

// Realtek RTL8139 network card support (10/100Mbit)
// (as per https://wiki.osdev.org/RTL8139, clones may be different)
// Copyright (C) 2023 Panagiotis

// Four TXAD register, you must use a different one to send packet each time(for
// example, use the first one, second... fourth and back to the first)
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

// Track current packet (when receiving)
uint32_t currentPacket;

// Defaults (should be 0.0.0.0 but whatever)
uint8_t defaultIP[4] = {10, 0, 2, 0};

bool isRTL8139(PCIdevice *device) {
  return (device->vendor_id == 0x10ec && device->device_id == 0x8139);
}

#define RTL8139_DEBUG 0

// todo: make an API which ensures each PCI device has it's own IRQ
void interruptHandler(AsmPassedInterrupt *regs) {
  rtl8139_interface *info = (rtl8139_interface *)selectedNIC->infoLocation;
  uint16_t           iobase = info->iobase;

  uint16_t status = inportw(iobase + RTL8139_REG_ISR);

  if (status & RTL8139_STATUS_TOK) {
#if RTL8139_DEBUG
    debugf("[networking//rtl8139//irq] Packet sent\n");
#endif
  }
  if (status & RTL8139_STATUS_ROK) {
#if RTL8139_DEBUG
    debugf("[networking//rtl8139//irq] Processing packet...\n");
#endif
    receiveRTL8139(selectedNIC);
  }
  outportw(info->iobase + RTL8139_REG_ISR, 0x5);
}

bool initiateRTL8139(PCIdevice *device) {
  if (!isRTL8139(device))
    return false;

  PCIgeneralDevice *details =
      (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
  GetGeneralDevice(device, details);

  uint16_t iobase = details->bar[0] & ~0x3;

  NIC *nic = createNewNIC();
  nic->type = RTL8139;
  nic->infoLocation = 0; // no extra info needed... yet.
  nic->irq = details->interruptLine;
  registerIRQhandler(nic->irq, &interruptHandler);

  rtl8139_interface *infoLocation =
      (rtl8139_interface *)malloc(sizeof(rtl8139_interface));
  nic->infoLocation = infoLocation;

  infoLocation->iobase = iobase;
  infoLocation->tx_curr = 0; // init this

  // Enable PCI Bus Mastering if it's not enabled already
  uint32_t command_status = combineWord(device->status, device->command);
  if (!(command_status & (1 << 2))) {
    command_status |= (1 << 2);
    ConfigWriteDword(device->bus, device->slot, device->function, PCI_COMMAND,
                     command_status);
  }

  // Turn the device on
  outportb(iobase + RTL8139_REG_POWERUP, 0x0);

  // Reset the device
  outportb(iobase + RTL8139_REG_CMD, 0x10);
  while ((inportb(iobase + RTL8139_REG_CMD) & 0x10) != 0) {
  }

  // Init the receive buffer
  PhysicallyContiguous all = VirtualAllocatePhysicallyContiguous(
      DivRoundUp(8192 + 16 + 1500, BLOCK_SIZE));
  void *virtual = all.virt;
  memset(virtual, 0, 8192 + 16 + 1500);
  void *physical = all.phys;
  outportl(iobase + RTL8139_REG_RBSTART, (uint32_t)physical);

  // Save it (physical can be computed if needed)
  infoLocation->rx_buff_virtual = virtual;
#if RTL8139_DEBUG
  debugf("virtual: %x physical: %x\n", virtual, physical);
#endif

  // Set the TOK and ROK bits high
  outportw(iobase + RTL8139_REG_IMR, 0x0005);

  // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
  outportl(iobase + 0x44, 0xf | (1 << 7));

  // Sets the RE and TE bits high
  outportb(iobase + RTL8139_REG_CMD, 0x0C);

  uint32_t MAC0_5 = inportl(iobase + RTL8139_REG_MAC0_5);
  uint16_t MAC5_6 = inportw(iobase + RTL8139_REG_MAC5_6);
  nic->MAC[0] = MAC0_5 >> 0;
  nic->MAC[1] = MAC0_5 >> 8;
  nic->MAC[2] = MAC0_5 >> 16;
  nic->MAC[3] = MAC0_5 >> 24;

  nic->MAC[4] = MAC5_6 >> 0;
  nic->MAC[5] = MAC5_6 >> 8;

  memcpy(nic->ip, defaultIP, 4);

  // waste of memory:
  // debugf("    [+] MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
  //        selectedNIC->MAC[0], selectedNIC->MAC[1], selectedNIC->MAC[2],
  //        selectedNIC->MAC[3], selectedNIC->MAC[4], selectedNIC->MAC[5]);

  free(details);

  return true;
}

void sendRTL8139(NIC *nic, void *packet, uint32_t packetSize) {
  rtl8139_interface *info = (rtl8139_interface *)nic->infoLocation;
  uint16_t           iobase = info->iobase;

  void *contiguousContainer =
      VirtualAllocatePhysicallyContiguous(DivRoundUp(packetSize, BLOCK_SIZE))
          .virt;
  void *phsyical = VirtualToPhysical((uint32_t)contiguousContainer);
  memcpy(contiguousContainer, packet, packetSize);

  outportl(iobase + TSAD_array[info->tx_curr], (uint32_t)phsyical);
  outportl(iobase + TSD_array[info->tx_curr++], packetSize);
  if (info->tx_curr > 3)
    info->tx_curr = 0;

  free(contiguousContainer); // the IRQ hits before this segement is reached
}

void receiveRTL8139(NIC *nic) {
  rtl8139_interface *info = (rtl8139_interface *)nic->infoLocation;
  uint16_t           iobase = info->iobase;

  uint16_t *buffer = (uint16_t *)(info->rx_buff_virtual + currentPacket);
  uint16_t  packetLength = *(buffer + 1);

  // we don't need the packet's pointer & length
  buffer += 2;

  void *packet = malloc(packetLength);
  memcpy(packet, buffer, packetLength);

  handlePacket(nic, packet, packetLength);
  free(packet);

  // waste of memory:
  // debugf("WE GOT A PACKET!\n");
  // for (int i = 0; i < packetLength; i++)
  //   debugf("%02X ", ((uint8_t *)packet)[i]);

  currentPacket = (currentPacket + packetLength + 4 + 3) & (~3);
  if (currentPacket > 8192)
    currentPacket -= 8192;

  outportw(iobase + 0x38, currentPacket - 0x10);
}
