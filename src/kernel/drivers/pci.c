#include <ahci.h>
#include <linked_list.h>
#include <malloc.h>
#include <nic_controller.h>
#include <pci.h>
#include <system.h>

// PCI driver
// Copyright (C) 2024 Panagiotis

/* PCI abstraction */

PCI *lookupPCIdevice(PCIdevice *device) {
  PCI *browse = firstPCI;
  while (browse) {
    if (browse->bus == device->bus && browse->slot == device->slot &&
        browse->function == device->function)
      break;
    browse = browse->next;
  }
  return browse;
}

void setupPCIdeviceDriver(PCI *pci, PCI_DRIVER driver,
                          PCI_DRIVER_CATEGORY category) {
  pci->driver = driver;
  pci->category = category;
}

/* Read/write to the PCI at certain offsets */

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

void ConfigWriteDword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                      uint32_t conf) {
  uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                (offset & 0xfc) | ((uint32_t)0x80000000));
  outportl(PCI_CONFIG_ADDRESS, address);
  outportl(PCI_CONFIG_DATA, conf);
}

/* Massive functions to output everything about a PCI device to an organized
 * struct */

int FilterDevice(uint8_t bus, uint8_t slot, uint8_t function) {
  uint16_t vendor_id = ConfigReadWord(bus, slot, function, 0x00);
  return !(vendor_id == 0xffff || !vendor_id);
}

void GetDevice(PCIdevice *device, uint8_t bus, uint8_t slot, uint8_t function) {
  device->bus = bus;
  device->slot = slot;
  device->function = function;

  device->vendor_id =
      ConfigReadWord(device->bus, device->slot, device->function, 0x00);
  device->device_id = ConfigReadWord(device->bus, device->slot,
                                     device->function, PCI_DEVICE_ID);

  device->command =
      ConfigReadWord(device->bus, device->slot, device->function, PCI_COMMAND);
  device->status =
      ConfigReadWord(device->bus, device->slot, device->function, PCI_STATUS);

  uint16_t revision_progIF = ConfigReadWord(device->bus, device->slot,
                                            device->function, PCI_REVISION_ID);
  device->revision = EXPORT_BYTE(revision_progIF, true);
  device->progIF = EXPORT_BYTE(revision_progIF, false);

  uint16_t subclass_class =
      ConfigReadWord(device->bus, device->slot, device->function, PCI_SUBCLASS);
  device->subclass_id = EXPORT_BYTE(subclass_class, true);
  device->class_id = EXPORT_BYTE(subclass_class, false);

  uint16_t cacheLineSize_latencyTimer = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_CACHE_LINE_SIZE);
  device->cacheLineSize = EXPORT_BYTE(cacheLineSize_latencyTimer, true);
  device->latencyTimer = EXPORT_BYTE(cacheLineSize_latencyTimer, false);

  uint16_t headerType_bist = ConfigReadWord(device->bus, device->slot,
                                            device->function, PCI_HEADER_TYPE);
  device->headerType = EXPORT_BYTE(headerType_bist, true);
  device->bist = EXPORT_BYTE(headerType_bist, false);
}

void GetGeneralDevice(PCIdevice *device, PCIgeneralDevice *out) {
  for (int i = 0; i < 6; i++)
    out->bar[i] =
        COMBINE_WORD(ConfigReadWord(device->bus, device->slot, device->function,
                                    PCI_BAR0 + 4 * i + 2),
                     ConfigReadWord(device->bus, device->slot, device->function,
                                    PCI_BAR0 + 4 * i));

  out->system_vendor_id = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_SYSTEM_VENDOR_ID);
  out->system_id = ConfigReadWord(device->bus, device->slot, device->function,
                                  PCI_SYSTEM_ID);

  out->expROMaddr =
      COMBINE_WORD(ConfigReadWord(device->bus, device->slot, device->function,
                                  PCI_EXP_ROM_BASE_ADDR + 2),
                   ConfigReadWord(device->bus, device->slot, device->function,
                                  PCI_EXP_ROM_BASE_ADDR));

  out->capabilitiesPtr =
      EXPORT_BYTE(ConfigReadWord(device->bus, device->slot, device->function,
                                 PCI_CAPABILITIES_PTR),
                  true);

  uint32_t interruptLine_interruptPIN = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_INTERRUPT_LINE);
  out->interruptLine = EXPORT_BYTE(interruptLine_interruptPIN, true);
  out->interruptPIN = EXPORT_BYTE(interruptLine_interruptPIN, false);

  uint32_t minGrant_maxLatency = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_MIN_GRANT);
  out->minGrant = EXPORT_BYTE(minGrant_maxLatency, true);
  out->maxLatency = EXPORT_BYTE(minGrant_maxLatency, false);
}

void initiatePCI() {
  PCIdevice *device = (PCIdevice *)malloc(sizeof(PCIdevice));

  for (uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
    for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
        if (!FilterDevice(bus, slot, function))
          continue;

        GetDevice(device, bus, slot, function);
        if ((device->headerType & ~(1 << 7)) != PCI_DEVICE_GENERAL)
          continue;

        PCI *target = LinkedListAllocate((void **)(&firstPCI), sizeof(PCI));
        target->bus = bus;
        target->slot = slot;
        target->function = function;

        target->vendor_id = device->vendor_id;
        target->device_id = device->device_id;

        switch (device->class_id) {
        case PCI_CLASS_CODE_NETWORK_CONTROLLER:
          initiateNIC(device);
          break;
        case PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER:
          if (device->subclass_id == 0x6)
            initiateAHCI(device);
          break;
        default:
          break;
        }
      }
    }
  }

  free(device);
}
