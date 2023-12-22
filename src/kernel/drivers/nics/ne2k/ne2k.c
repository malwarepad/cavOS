#include <ne2k.h>
#include <nic_controller.h>

// Ne2000 network interface driver
// Copyright (C) 2023 Panagiotis

bool isNe2000(PCIdevice *device) {
  return (device->vendor_id == 0x10ec && device->device_id == 0x8029);
}

bool initiateNe2000(PCIdevice *device) {
  if (!isNe2000(device))
    return false;

  debugf("[+] NICs: ne2k detected!\n");

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

  outportb(iobase, (1 << 5) | 1); // page 0, no DMA, stop
  outportb(iobase + 0x0E, 0x49);  // set word-wide access
  outportb(iobase + 0x0A, 0);     // clear the count regs
  outportb(iobase + 0x0B, 0);     //
  outportb(iobase + 0x0F, 0);     // mask completion IRQ
  outportb(iobase + 0x07, 0xFF);  //
  outportb(iobase + 0x0C, 0x20);  // set to monitor
  outportb(iobase + 0x0D, 0x02);  // and loopback mode.
  outportb(iobase + 0x0A, 32);    // reading 32 bytes
  outportb(iobase + 0x0B, 0);     // count high
  outportb(iobase + 0x08, 0);     // start DMA at 0
  outportb(iobase + 0x09, 0);     // start DMA high
  outportb(iobase, 0x0A);         // start the read

  NIC *nic = createNewNIC();
  nic->type = NE2000;
  nic->infoLocation = 0; // no extra info needed... yet.

  for (int i = 0; i < 32; i++) {
    // prom[i] = inportb(iobase + 0x10);
    if (i < 6)
      nic->MAC[i] = inportb(iobase + 0x10);
    else
      inportb(iobase + 0x10);
  };

  debugf("    [+] MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
         selectedNIC->MAC[0], selectedNIC->MAC[1], selectedNIC->MAC[2],
         selectedNIC->MAC[3], selectedNIC->MAC[4], selectedNIC->MAC[5]);

  free(details);

  return true;
}
