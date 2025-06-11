#include <apic.h>
#include <bootloader.h>
#include <e1000.h>
#include <isr.h>
#include <malloc.h>
#include <system.h>

#include <util.h>

#include <paging.h>
#include <pmm.h>
#include <vmm.h>

// Intel E1000 network card support (100/1000Mbit)
// Don't be fooled, the OSDev article is for the e1000e not the e1000!
// Copyright (C) 2025 Panagiotis

// #define debugf(...) ((void)0)

bool isE1000(PCIdevice *device) {
  return (device->vendor_id == 0x8086 && device->device_id == 0x100e) || // qemu
         (device->vendor_id == 0x8086 && device->device_id == 0x100f); // vmware

  // I am confident more will work, however I don't own any so I won't risk it
}

uint32_t E1000CmdRead(E1000_interface *e1000, uint16_t addr) {
  if (e1000->membase) {
    uint32_t volatile *ptr = (uint32_t volatile *)(e1000->membase + addr);
    return *ptr;
  } else {
    outportl(e1000->iobase, addr);
    return inportl(e1000->iobase + 4);
  }
}

void E1000CmdWrite(E1000_interface *e1000, uint16_t addr, uint32_t value) {
  if (e1000->membase) {
    uint32_t volatile *ptr = (uint32_t volatile *)(e1000->membase + addr);
    *ptr = value;
  } else {
    outportl(e1000->iobase, addr);
    outportl(e1000->iobase + 4, value);
  }
}

void E1000EepromDetect(E1000_interface *e1000) {
  if (e1000->deviceId == 0x1107 || e1000->deviceId == 0x1112)
    return;

  e1000->eeprom = E1000CmdRead(e1000, REG_EECD) & EECD_EEPROM_PRESENT;
}

bool is_82541xx(uint16_t deviceId) {
  return deviceId == 0x1013 || deviceId == 0x1018 || deviceId == 0x1076 ||
         deviceId == 0x1077 || deviceId == 0x1078;
};

bool is_82547_GI_EI(uint16_t deviceId) {
  return deviceId == 0x101a || deviceId == 0x1019;
}

/// Use this to prepare an address to store into the EERD register.
uint32_t EERD_ADDRESS(uint8_t address) { return (uint32_t)(address) << 8; };
uint32_t EERD_ADDRESS_EXTRA(uint8_t address) {
  return (uint32_t)(address) << 2;
};
/// Get 16-bit data word from EERD register after a read is done.
uint16_t EERD_DATA(uint32_t eerd) { return (uint16_t)(eerd >> 16); };

uint16_t E1000EepromRead(E1000_interface *e1000, uint8_t addr) {
  uint32_t calculatedAddress = 0;
  uint32_t successMask = 0;
  if (is_82541xx(e1000->deviceId) || is_82547_GI_EI(e1000->deviceId)) {
    calculatedAddress = EERD_ADDRESS_EXTRA(addr);
    successMask = EERD_DONE_EXTRA;
  } else {
    calculatedAddress = EERD_ADDRESS(addr);
    successMask = EERD_DONE;
  }
  /// TODO: If EECD register indicates that software has direct pin
  /// control of the EEPROM, access through the EERD register can stall
  /// until that bit is clear. Software should ensure that EECD.EE_REQ
  /// and EECD.EE_GNT bits are clear before attempting to use EERD to
  /// access the EEPROM.
  // FIXME: This hangs forever (am I doing something wrong?)
  // while (read_command(REG_EECD) & (EECD_EEPROM_REQUEST | EECD_EEPROM_GRANT));

  /// Write address to EEPROM register along with the Start Read bit
  /// to indicate to the NIC that it needs to do an EEPROM read with
  /// given address.
  E1000CmdWrite(e1000, REG_EEPROM, EERD_START | calculatedAddress);

  // TODO: Maybe put a max spin so we don't hang forever in case of
  // something going wrong!
  // Read from EEPROM register until READ_DONE bit is set.
  uint32_t out = 0;
  do
    out = E1000CmdRead(e1000, REG_EEPROM);
  while (!(out & successMask));

  return EERD_DATA(out);
}

void E1000RXConfigure(E1000_interface *e1000) {
  // put down our desired MAC address (the default) on RAL
  if (e1000->membase) {
    volatile uint8_t *base =
        (volatile uint8_t *)(e1000->membase + REG_RAL_BEGIN);
    for (int i = 0; i < 6; i++)
      base[i] = e1000->nic->MAC[i];
  } else {
    assert(false);
  }

  // allocate some space for the receive list
  e1000->rxList = VirtualAllocate(E1000_RX_PAGE_COUNT);
  memset(e1000->rxList, 0, E1000_RX_PAGE_COUNT * PAGE_SIZE);
  size_t rxListPhys = VirtualToPhysical((size_t)e1000->rxList);
  E1000CmdWrite(e1000, REG_RXDESCLO, SPLIT_64_LOWER(rxListPhys));
  E1000CmdWrite(e1000, REG_RXDESCHI, SPLIT_64_HIGHER(rxListPhys));
  E1000CmdWrite(e1000, REG_RXDESCLEN, E1000_RX_PAGE_COUNT * PAGE_SIZE);

  // every descriptor in this range is owned by hardware
  E1000CmdWrite(e1000, REG_RXDESCHEAD, 0);
  E1000CmdWrite(e1000, REG_RXDESCTAIL, E1000_RX_LIST_ENTRIES);

  // actually allocate the buffers
  for (int i = 0; i < E1000_RX_LIST_ENTRIES; i++) {
    // zero'd before so we good (8KiB per entry)
    e1000->rxList[i].addr = VirtualToPhysical((size_t)VirtualAllocate(2));
  }

  E1000CmdWrite(
      e1000, REG_RX_CONTROL,
      RCTL_LOOPBACK_MODE_OFF | RCTL_BROADCAST_ACCEPT_MODE |
          RCTL_LONG_PACKET_RECEPTION_ENABLE | RCTL_UNICAST_PROMISCUOUS_ENABLED |
          RCTL_MULTICAST_PROMISCUOUS_ENABLED |
          RCTL_DESC_MIN_THRESHOLD_SIZE_HALF | RCTL_STRIP_ETHERNET_CRC |
          RCTL_STORE_BAD_PACKETS | RCTL_BUFFER_SIZE_8192);
}

void E1000TXConfigure(E1000_interface *e1000) {
  // allocate some space for the transmit list
  e1000->txList = VirtualAllocate(E1000_TX_PAGE_COUNT);
  memset(e1000->txList, 0, E1000_TX_PAGE_COUNT * PAGE_SIZE);
  size_t txListPhys = VirtualToPhysical((size_t)e1000->txList);
  E1000CmdWrite(e1000, REG_TXDESCLO, SPLIT_64_LOWER(txListPhys));
  E1000CmdWrite(e1000, REG_TXDESCHI, SPLIT_64_HIGHER(txListPhys));
  E1000CmdWrite(e1000, REG_TXDESCLEN, E1000_TX_PAGE_COUNT * PAGE_SIZE);

  // every descriptor in this range is owned by software
  E1000CmdWrite(e1000, REG_TXDESCHEAD, 0);
  E1000CmdWrite(e1000, REG_TXDESCTAIL, 0);
}

void E1000InterruptHandler() {
  E1000_interface *e1000 = selectedNIC->infoLocation; // todo: bad for multiple

  uint32_t status = E1000CmdRead(e1000, REG_ICR);

  if (status & ICR_RX_OVERRUN || status & ICR_RX_TIMER_INTERRUPT) {
    status &= ~(ICR_RX_OVERRUN | ICR_RX_TIMER_INTERRUPT);
    // uint32_t head = E1000CmdRead(e1000, REG_RXDESCHEAD);
    for (e1000->rxHead = 0; e1000->rxHead < E1000_RX_LIST_ENTRIES;
         e1000->rxHead++) {
      volatile E1000RX *rxDesc = &e1000->rxList[e1000->rxHead];
      if (rxDesc->status & E1000RX_STATUS_DONE &&
          rxDesc->status & E1000RX_STATUS_END_OF_PACKET) {
        netQueueAdd(selectedNIC,
                    (uint8_t *)(bootloader.hhdmOffset + rxDesc->addr),
                    rxDesc->length);
        rxDesc->status = 0;
        rxDesc->errors = 0;
      }

      // if (!(rxDesc->status & E1000RX_STATUS_DONE) || rxDesc->errors) {
      //   debugf("[pci::e1000] Packet dropped from being received! status{%x} "
      //          "errors(%x)\n",
      //          rxDesc->status, rxDesc->errors);
      //   rxDesc->status = 0;
      //   rxDesc->errors = 0;
      //   continue; // todo <- continue
      // }

      // rxDesc->status = 0;
      // rxDesc->errors = 0;
    }
  }

  if (status & ICR_TX_DESC_WRITTEN_BACK) {
    status &= ~ICR_TX_DESC_WRITTEN_BACK;
    // transmit succeeded
  }

  if (status & ICR_TX_QUEUE_EMPTY) {
    status &= ~ICR_TX_QUEUE_EMPTY;
    // will be frequently hit, nothing to worry about
  }

  if (status & ICR_LINK_STATUS_CHANGE) {
    status &= ~ICR_LINK_STATUS_CHANGE;
    E1000CmdWrite(e1000, REG_CTRL,
                  E1000CmdRead(e1000, REG_CTRL) | CTRL_SET_LINK_UP);
    debugf("[pci::e1000] Link status has been changed!\n");
  }

  if (status & ICR_RX_SEQUENCE_ERROR) {
    status &= ~ICR_RX_SEQUENCE_ERROR;
    debugf("[pci::e1000] Sequence error hit!\n");
  }

  if (status & ICR_RX_DESC_MIN_THRESHOLD_HIT) {
    status &= ~ICR_RX_DESC_MIN_THRESHOLD_HIT;
    // we prolly need more of those atp
  }

  if (status & ICR_TX_DESC_MIN_THRESHOLD_HIT) {
    status &= ~ICR_TX_DESC_MIN_THRESHOLD_HIT;
    // we prolly need more of those atp
  }

  if (status)
    debugf("[pci::e1000] Unhandled status bits: status{%x}\n", status);

  // control
  if (E1000CmdRead(e1000, REG_RXDESCHEAD) ==
      E1000CmdRead(e1000, REG_RXDESCTAIL))
    E1000CmdWrite(e1000, REG_RXDESCHEAD, 0);

  // (void)E1000CmdRead(e1000, REG_ICR); // apparently this is necessary
}

void sendE1000(NIC *nic, void *packet, uint32_t packetSize) {
  E1000_interface *e1000 = nic->infoLocation;

  uint32_t          tail = E1000CmdRead(e1000, REG_TXDESCTAIL);
  volatile E1000TX *desc = &e1000->txList[tail];

  size_t pages = DivRoundUp(packetSize, PAGE_SIZE);
  void  *virt = VirtualAllocatePhysicallyContiguous(pages);

  desc->addr = VirtualToPhysical((size_t)virt);
  memcpy(virt, packet, packetSize);
  desc->length = packetSize;
  desc->command = CMD_EOP | CMD_IFCS | CMD_RS;
  desc->status = 0;

  tail++;
  uint32_t tx_desc_count = E1000CmdRead(e1000, REG_TXDESCLEN) / sizeof(E1000TX);
  if (tx_desc_count)
    tail %= tx_desc_count;
  E1000CmdWrite(e1000, REG_TXDESCTAIL, tail);

  while (!desc->status)
    handControl(); // wait for it to be done
}

bool initiateE1000(PCIdevice *device) {
  if (!isE1000(device))
    return false;

  PCIgeneralDevice *details =
      (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
  GetGeneralDevice(device, details);

  // Enable PCI Bus Mastering, memory access and interrupts (if not already)
  uint32_t command_status = COMBINE_WORD(device->status, device->command);
  if (!(command_status & (1 << 2)))
    command_status |= (1 << 2); // PCI Bus Mastering
  if (!(command_status & (1 << 1)))
    command_status |= (1 << 1); // PCI Memory Space
  if (command_status & (1 << 10))
    command_status &= ~(1 << 10); // PCI Interrupt Disable
  // command_status |= (1 << 10);
  ConfigWriteDword(device->bus, device->slot, device->function, PCI_COMMAND,
                   command_status);

  PCI *pci = lookupPCIdevice(device);
  setupPCIdeviceDriver(pci, PCI_DRIVER_E1000, PCI_DRIVER_CATEGORY_NIC);

  NIC *nic = createNewNIC(pci);
  nic->type = E1000;
  nic->mintu = 60;
  nic->infoLocation = 0; // no extra info needed... yet.
  nic->irq = details->interruptLine;

  debugf("[pci::e1000] Intel E1000 NIC detected! dev{%x}\n", device->device_id);

  E1000_interface *infoLocation =
      (E1000_interface *)malloc(sizeof(E1000_interface));
  memset(infoLocation, 0, sizeof(E1000_interface));
  nic->infoLocation = infoLocation;
  infoLocation->nic = nic; // yeah it needs to point back

  infoLocation->deviceId = device->device_id;

  // how will we interface with the device?
  if (details->bar[0] & (1 << 0)) {
    infoLocation->iobase = details->bar[0] & ~0x3;
    debugf("[pci::e1000] Fetched BAR[0] and I/O ports are used: iobase{%lx}\n",
           infoLocation->iobase);
  } else {
    infoLocation->membasePhys = details->bar[0] & ~15;
    infoLocation->membase = bootloader.hhdmOffset + infoLocation->membasePhys;
    debugf("[pci::e1000] Fetched BAR[0] and MMIO is used: membase{%lx}\n",
           infoLocation->membasePhys);
  }

  E1000EepromDetect(infoLocation);
  if (!infoLocation->eeprom) {
    debugf("[pci::e1000] Todo: MAC parsing without EEPROM!\n");
    panic();
  }

  // read the EEPROM to get our MAC address
  uint16_t value = 0;
  value = E1000EepromRead(infoLocation, EEPROM_ETHERNET_ADDRESS_BYTES0);
  nic->MAC[0] = value & 0xff;
  nic->MAC[1] = value >> 8;
  value = E1000EepromRead(infoLocation, EEPROM_ETHERNET_ADDRESS_BYTES1);
  nic->MAC[2] = value & 0xff;
  nic->MAC[3] = value >> 8;
  value = E1000EepromRead(infoLocation, EEPROM_ETHERNET_ADDRESS_BYTES2);
  nic->MAC[4] = value & 0xff;
  nic->MAC[5] = value >> 8;

  // configure the device
  uint32_t control = E1000CmdRead(infoLocation, REG_CTRL);
  control &= ~CTRL_LINK_RESET; // link reset 0b = normal (1b disables auto neg)
  control &= ~CTRL_PHY_RESET;  // physical reset 0b
  control &= ~CTRL_VLAN_MODE_ENABLE; // no vlans

  // CTRL.ILOS should be set to 0b (not applicable to the
  // 82541xx and 82547GI/EI).
  if (!is_82541xx(infoLocation->deviceId) &&
      !is_82547_GI_EI(infoLocation->deviceId))
    control &= ~CTRL_INVERT_LOSS_OF_SIGNAL;

  // start the link up
  control |= CTRL_SET_LINK_UP;
  E1000CmdWrite(infoLocation, REG_CTRL, control);

  E1000RXConfigure(infoLocation);
  E1000TXConfigure(infoLocation);

  uint8_t targIrq = ioApicPciRegister(device, details);
  pci->irqHandler = registerIRQhandler(targIrq, &E1000InterruptHandler);

  // configure interrupts
  E1000CmdWrite(infoLocation, REG_IMASK_CLEAR, 0xffffffff);
  E1000CmdWrite(infoLocation, REG_IMASK,
                IMASK_RX_TIMER_INTERRUPT | IMASK_RX_OVERRUN |
                    IMASK_RX_DESC_MIN_THRESHOLD_HIT | IMASK_RX_SEQUENCE_ERROR |
                    IMASK_LINK_STATUS_CHANGE | IMASK_TX_QUEUE_EMPTY |
                    IMASK_TX_DESC_WRITTEN_BACK |
                    IMASK_TX_DESC_MIN_THRESHOLD_HIT);

  // discard any pending interrupts
  E1000CmdRead(infoLocation, REG_ICR);

  // enable receiving
  E1000CmdWrite(infoLocation, REG_RX_CONTROL,
                E1000CmdRead(infoLocation, REG_RX_CONTROL) | RCTL_ENABLE);

  // enable transmiting
  E1000CmdWrite(infoLocation, REG_TCTL,
                E1000CmdRead(infoLocation, REG_TCTL) | TCTL_EN);

  return true;
}
