#include <dhcp.h>
#include <isr.h>
#include <liballoc.h>
#include <paging.h>
#include <pmm.h>
#include <rtl8169.h>
#include <system.h>
#include <util.h>
#include <vmm.h>

// Realtek RTL8169 network card support (10/100/1000Mbit)
// (as per https://wiki.osdev.org/RTL8169, clones may be different)
// Copyright (C) 2024 Panagiotis

#define DEBUG_RTL8169 0

// From FreeBSD... idk the accurancy of this, idk if my implementation is
// generic enough for this buuuuut here we are!
bool isRTL8169(PCIdevice *device) {
  return ((device->vendor_id == 0x10ec &&
           (device->device_id == 0x8161 || device->device_id == 0x8168 ||
            device->device_id == 0x8169)) ||
          (device->vendor_id == 0x1259 && device->device_id == 0xc107) ||
          (device->vendor_id == 0x1737 && device->device_id == 0x1032) ||
          (device->vendor_id == 0x16ec && device->device_id == 0x0116));
}

void interruptHandlerRTL8169(AsmPassedInterrupt *regs) {
#if DEBUG_RTL8169
  printf("[pci::rtl8169] Interrupt!\n");
#endif
  rtl8169_interface *info = (rtl8169_interface *)selectedNIC->infoLocation;
  uint16_t           iobase = info->iobase;

  uint16_t status = inportw(iobase + 0x3E);

  bool dhcpInit = false;
  if (status & RTL8169_LINK_CHANGE) {
    status |= RTL8169_LINK_CHANGE;
    dhcpInit = true;
#if DEBUG_RTL8169
    printf("[pci::rtl8169] Link change detected!\n");
#endif
  }

  if (status & RTL8169_RECV) {
#if DEBUG_RTL8169
    printf("[pci::rtl8169] Received!\n");
#endif

    for (int i = 0; i < RTL8169_RX_DESCRIPTORS; i++) {
      if (info->RxDescriptors[i].command & RTL8169_OWN)
        continue;

      uint32_t buffSize = info->RxDescriptors[i].command & 0x3FFF;
      uint32_t low = info->RxDescriptors[i].low_buf;
      uint32_t high = info->RxDescriptors[i].high_buf;

      handlePacket(selectedNIC, info->packetBuffers[i], buffSize);

      info->RxDescriptors[i].command |= RTL8169_OWN;
    }

    status |= RTL8169_RECV;
  }

  if (status & RTL8169_SENT) {
#if DEBUG_RTL8169
    printf("[pci::rtl8169] Sent!\n");
#endif
    info->txSent = true;
    status |= RTL8169_SENT;
  }

  outportw(iobase + 0x3E, status);

  status = inportw(iobase + 0x3E);
  if (status != 0x00) {
#if DEBUG_RTL8169
    printf("[pci::rtl8169] Unresolved interrupt: %x \n", status);
#endif
  }

  if (dhcpInit)
    netDHCPinit(selectedNIC);
}

void sendRTL8169(NIC *nic, void *packet, uint32_t packetSize) {
  rtl8169_interface *info = (rtl8169_interface *)nic->infoLocation;
  uint16_t           iobase = info->iobase;

  uint32_t cmd = RTL8169_OWN | RTL8169_EOR | 0x40000 | 0x20000000 | 0x10000000 |
                 (packetSize & 0x3FFF);

  rtl8169_descriptor *desc =
      ((rtl8169_descriptor *)((uint32_t)info->TxDescriptors +
                              (sizeof(rtl8169_descriptor) * 0)));

  uint32_t allocSize = DivRoundUp(packetSize, BLOCK_SIZE);
  void    *contiguousContainer =
      VirtualAllocatePhysicallyContiguous(allocSize).virt;
  void *phsyical = VirtualToPhysical((uint32_t)contiguousContainer);
  memcpy(contiguousContainer, packet, packetSize);

  desc->high_buf = 0; // nah, we're on x86
  desc->low_buf = (uint32_t)phsyical;
  desc->vlan = 0;
  desc->command = cmd;

  info->txSent = false;
  outportb(iobase + 0x38, 0x40);

  while (!info->txSent && (inportb(iobase + 0x38) & 0x40))
    ;
  info->txSent = false;

  VirtualFree(contiguousContainer, allocSize);
}

bool initiateRTL8169(PCIdevice *device) {
#if DEBUG_RTL8169
  printf("Checking for RTL8169s...\n");
#endif
  if (!isRTL8169(device))
    return false;

  debugf("[pci::rtl8169] RTL-8169 NIC detected!\n");

  PCIgeneralDevice *details =
      (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
  GetGeneralDevice(device, details);

  for (int i = 0; i < 5; i++) {
    uint16_t iobase = details->bar[i] & 0xFFFFFFFE;
#if DEBUG_RTL8169
    printf("[pci::rtl8169] [bar%d] iobase: %x\n", i, iobase);
#endif
  }

  uint16_t iobase = details->bar[0] & 0xFFFFFFFE;

  NIC *nic = createNewNIC();
  nic->type = RTL8169;
  nic->infoLocation = 0; // no extra info needed... yet.
  nic->irq = details->interruptLine;

  rtl8169_interface *infoLocation =
      (rtl8169_interface *)malloc(sizeof(rtl8169_interface));
  memset(infoLocation, 0, sizeof(rtl8169_interface));
  nic->infoLocation = infoLocation;

  infoLocation->iobase = iobase;
  PhysicallyContiguous rxDesc = VirtualAllocatePhysicallyContiguous(DivRoundUp(
      sizeof(rtl8169_descriptor) * RTL8169_RX_DESCRIPTORS, BLOCK_SIZE));
  infoLocation->RxDescriptors = (rtl8169_descriptor *)rxDesc.virt;
  PhysicallyContiguous txDesc = VirtualAllocatePhysicallyContiguous(DivRoundUp(
      sizeof(rtl8169_descriptor) * RTL8169_TX_DESCRIPTORS, BLOCK_SIZE));
  infoLocation->TxDescriptors = (rtl8169_descriptor *)txDesc.virt;

  uint32_t command_status = combineWord(device->status, device->command);
  if (!(command_status & (1 << 2))) {
    command_status |= (1 << 2);
    ConfigWriteDword(device->bus, device->slot, device->function, PCI_COMMAND,
                     command_status);
  }

  registerIRQhandler(nic->irq, &interruptHandlerRTL8169);

  outportb(iobase + 0x37, 0x10);
  while (inportb(iobase + 0x37) & 0x10) {
  }

  for (int i = 0; i < 6; i++)
    nic->MAC[i] = inportb(iobase + i);

#if DEBUG_RTL8169
  printf("[pci::rtl8169] MAC{%02X:%02X:%02X:%02X:%02X:%02X}\n", nic->MAC[0],
         nic->MAC[1], nic->MAC[2], nic->MAC[3], nic->MAC[4], nic->MAC[5]);
#endif

  for (uint32_t i = 0; i < RTL8169_RX_DESCRIPTORS; i++) {
    uint32_t rx_buffer_len = 1536;
    uint32_t packet_buffer_address = (uint32_t)malloc(rx_buffer_len);
    infoLocation->packetBuffers[i] = packet_buffer_address;
    if (i == (RTL8169_RX_DESCRIPTORS - 1)) {
      infoLocation->RxDescriptors[i].command =
          (RTL8169_OWN | RTL8169_EOR | (rx_buffer_len & 0x3FFF));
    } else {
      infoLocation->RxDescriptors[i].command =
          (RTL8169_OWN | (rx_buffer_len & 0x3FFF));
    }
    infoLocation->RxDescriptors[i].low_buf =
        (uint32_t)VirtualToPhysical(packet_buffer_address);
    infoLocation->RxDescriptors[i].high_buf = 0;
  }

#if DEBUG_RTL8169
  printf("[pci::rtl8169] Starting configuration...\n");
#endif

  outportb(iobase + 0x50, 0xC0); /* Unlock config registers */
  outportl(iobase + 0x44,
           0x0000E70F); /* RxConfig = RXFTH: unlimited, MXDMA: unlimited, AAP:
                           set (promisc. mode set) */
  outportb(iobase + 0x37, 0x04); /* Enable Tx in the Command register, required
                                  before setting TxConfig */
  outportl(iobase + 0x40,
           0x03000700); /* TxConfig = IFG: normal, MXDMA: unlimited */
  outportw(iobase + 0xDA, 0x1FFF); /* Max rx packet size */
  outportb(iobase + 0xEC, 0x3B);   /* max tx packet size */
#if DEBUG_RTL8169
  printf("[pci::rtl8169] Registering TX & RX descriptors...\n");
#endif
  outportl(iobase + 0x20,
           txDesc.phys); /* Tell the NIC where the first Tx descriptor is */
  outportl(iobase + 0x24, 0);
  outportl(iobase + 0xE4,
           rxDesc.phys); /* Tell the NIC where the first Rx descriptor is */
  outportl(iobase + 0xE8, 0);
  outportw(iobase + 0x3C, 0xC1FF); /* Set all masks open so we get much ints */
  outportb(iobase + 0x37, 0x0C);   /* Enable Rx/Tx in the Command register */
  outportb(iobase + 0x50, 0x00);   /* Lock config registers */

#if DEBUG_RTL8169
  printf("[pci::rtl8169] Somehow finished configuration!\n");
#endif

  free(details);

  return true;
}
