#include <ne2k.h>
#include <system.h>

// Ne2000 network interface driver
// Copyright (C) 2023 Panagiotis

bool isNe2000(PCIdevice *device) {
  return (device->vendor_id == 0x10ec && device->device_id == 0x8029);
}

bool initiateNe2000(PCIdevice *device) {
  if (!isNe2000(device))
    return false;

#ifndef BUGGY_NE2k
  debugf("[+] Ne2k: ignored\n");
  return false;
#endif

  PCIgeneralDevice *details =
      (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
  GetGeneralDevice(device, details);

  uint16_t iobase = details->bar[0] & ~0x3;

  outportb(iobase + 0x1F,
           inportb(iobase +
                   0x1F)); // write the value of RESET into the RESET register
  while ((inportb(iobase + 0x07) & 0x80) == 0)
    ;                            // wait for the RESET to complete
  outportb(iobase + 0x07, 0xFF); // mask interrupts

  outportb(iobase + NE2K_REG_COMMAND, (1 << 5) | 1); // page 0, no DMA, stop
  outportb(iobase + NE2K_REG_DCR, 0x49);             // set word-wide access
  outportb(iobase + NE2K_REG_RBCR0, 0);              // clear the count regs
  outportb(iobase + NE2K_REG_RBCR1, 0);              //
  outportb(iobase + NE2K_REG_IMR, 0);                // mask completion IRQ
  outportb(iobase + NE2K_REG_ISR, 0xFF);             //
  outportb(iobase + NE2K_REG_RCR, 0x20);             // set to monitor
  outportb(iobase + NE2K_REG_TCR, 0x02);             // and loopback mode.
  outportb(iobase + NE2K_REG_RBCR0, 32);             // reading 32 bytes
  outportb(iobase + NE2K_REG_RBCR1, 0);              // count high
  outportb(iobase + NE2K_REG_RSAR0, 0);              // start DMA at 0
  outportb(iobase + NE2K_REG_RSAR1, 0);              // start DMA high
  outportb(iobase + NE2K_REG_COMMAND, 0x0A);         // start the read

  NIC *nic = createNewNIC();
  nic->type = NE2000;
  nic->infoLocation = 0; // no extra info needed... yet.

  ne2k_interface *infoLocation =
      (ne2k_interface *)malloc(sizeof(ne2k_interface));
  nic->infoLocation = infoLocation;

  infoLocation->iobase = iobase;

  for (int i = 0; i < 32; i++) {
    // prom[i] = inportb(iobase + 0x10);
    if (i < 6)
      nic->MAC[i] = inportb(iobase + NE2K_REG_DATA);
    else
      inportb(iobase + NE2K_REG_DATA);
  };

  // Listen from MAC addr
  for (int i = 0; i < 6; i++) {
    outportb(iobase + 1 + i, selectedNIC->MAC[i]);
  }

  outportb(iobase + NE2K_REG_COMMAND, (1 << 5) | 1); // page 0, no DMA, stop

  // waste of memory:
  // debugf("    [+] MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
  //        selectedNIC->MAC[0], selectedNIC->MAC[1], selectedNIC->MAC[2],
  //        selectedNIC->MAC[3], selectedNIC->MAC[4], selectedNIC->MAC[5]);
  debugf("irq%d\n", details->interruptLine);

  free(details);

  return true;
}

void sendNe2000(NIC *nic, void *packet, uint32_t packetSize) {
#ifndef BUGGY_NE2k
  debugf("[+] Ne2k: ignored\n");
  return false;
#endif

  ne2k_interface *info = (ne2k_interface *)nic->infoLocation;
  uint16_t        iobase = info->iobase;

  outportb(iobase + NE2K_REG_COMMAND, 0x22); // COMMAND register start & rodma
  outportb(iobase + NE2K_REG_RBCR0, packetSize & 0xFF); // load packet size
  outportb(iobase + NE2K_REG_RBCR1, packetSize >> 8);   // load packet size
  outportb(iobase + NE2K_REG_ISR, (1UL << 6UL));        // remote DMA complete?
  outportb(iobase + NE2K_REG_RSAR0, 0x00);              // zero out RSAR0
  outportb(iobase + NE2K_REG_RSAR1, 0x00);              // page number (0) high
  outportb(iobase + NE2K_REG_COMMAND, 0x12);            // start remote w DMA

  uint8_t *rawPacket = (uint8_t *)packet;
  for (int i = 0; i < packetSize; i++)
    outportb(iobase + NE2K_REG_DATA, rawPacket[i]);

  while ((inportb(iobase + NE2K_REG_ISR) & (1UL << 6UL)) == 0)
    ; // poll ISR register until bit 6 (Remote DMA completed) is set.
}
