#include "../../include/pci.h"
#include "../../include/system.h"

// PCI driver
// Copyright (C) 2023 Panagiotis

uint16_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                        uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  uint16_t tmp = 0;

  // Create configuration address as per Figure 1
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address
  outportl(PCI_CONFIG_ADDRESS, address);
  // Read in the data
  // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
  tmp = (uint16_t)((inportl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
  return tmp;
}

uint16_t getClassId(uint16_t bus, uint16_t device, uint16_t function) {
  uint32_t r0 = ConfigReadWord(bus, device, function, PCI_SUBCLASS);
  return (r0 & ~0x00FF) >> 8;
}

uint16_t getSubClassId(uint16_t bus, uint16_t device, uint16_t function) {
  uint32_t r0 = ConfigReadWord(bus, device, function, PCI_SUBCLASS);
  return (r0 & ~0xFF00);
}

int FilterDevice(uint8_t bus, uint8_t slot, uint8_t function) {
  uint16_t vendor_id = ConfigReadWord(bus, slot, function, 0x00);
  return !(vendor_id == 0xffff || !vendor_id);
}

void GetDeviceDetails(PCIdevice *device, uint8_t details, uint8_t bus,
                      uint8_t slot, uint8_t function) {
  device->bus = bus;
  device->slot = slot;
  device->function = function;

  device->class_id = getClassId(bus, slot, function);
  device->subclass_id = getSubClassId(bus, slot, function);

  if (!details)
    return;

  device->vendor_id = ConfigReadWord(bus, slot, function, 0x00);
  device->device_id = ConfigReadWord(bus, slot, function, PCI_DEVICE_ID);

  device->interface_id = ConfigReadWord(bus, slot, function, PCI_PROG_IF);

  device->system_vendor_id =
      ConfigReadWord(bus, slot, function, PCI_SYSTEM_VENDOR_ID);
  device->system_id = ConfigReadWord(bus, slot, function, PCI_SYSTEM_ID);

  device->revision = ConfigReadWord(bus, slot, function, PCI_REVISION_ID);
  device->interrupt = ConfigReadWord(bus, slot, function, PCI_INTERRUPT_LINE);
}

void initiatePCI() {
  for (uint8_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
    for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
        if (!FilterDevice(bus, slot, function))
          continue;

        PCIdevice *device = (PCIdevice *)malloc(sizeof(PCIdevice));
        GetDeviceDetails(device, 0, bus, slot, function);
        switch (device->class_id) {
        case PCI_CLASS_CODE_NETWORK_CONTROLLER:
          break;
        default:
          break;
        }
        free(device);
      }
    }
  }
}
