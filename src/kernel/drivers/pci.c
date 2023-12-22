#include <nic_controller.h>
#include <pci.h>
#include <system.h>

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

uint8_t exportByte(uint32_t target, bool first) {
  if (first)
    return target & ~0xFF00;
  else
    return (target & ~0x00FF) >> 8;
}

uint32_t combineWord(uint16_t msb, uint16_t lsb) { // 0-7 -> msb, 8-15 -> lsb
  uint32_t result = ((uint32_t)msb << 16) | lsb;

  return result;
}

uint8_t getClassId(uint16_t bus, uint16_t device, uint16_t function) {
  uint32_t r0 = ConfigReadWord(bus, device, function, PCI_SUBCLASS);
  return exportByte(r0, false);
  // return (r0 & ~0x00FF) >> 8;
}

uint8_t getSubClassId(uint16_t bus, uint16_t device, uint16_t function) {
  uint32_t r0 = ConfigReadWord(bus, device, function, PCI_SUBCLASS);
  return exportByte(r0, true);
  // return (r0 & ~0xFF00);
}

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
  device->revision = exportByte(revision_progIF, true);
  device->progIF = exportByte(revision_progIF, false);

  uint16_t subclass_class =
      ConfigReadWord(device->bus, device->slot, device->function, PCI_SUBCLASS);
  device->subclass_id = exportByte(subclass_class, true);
  device->class_id = exportByte(subclass_class, false);

  uint16_t cacheLineSize_latencyTimer = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_CACHE_LINE_SIZE);
  device->cacheLineSize = exportByte(cacheLineSize_latencyTimer, true);
  device->latencyTimer = exportByte(cacheLineSize_latencyTimer, false);

  uint16_t headerType_bist = ConfigReadWord(device->bus, device->slot,
                                            device->function, PCI_HEADER_TYPE);
  device->headerType = exportByte(headerType_bist, true);
  device->bist = exportByte(headerType_bist, false);
}

void GetGeneralDevice(PCIdevice *device, PCIgeneralDevice *out) {
  for (int i = 0; i < 6; i++)
    out->bar[i] =
        combineWord(ConfigReadWord(device->bus, device->slot, device->function,
                                   PCI_BAR0 + 4 * i + 2),
                    ConfigReadWord(device->bus, device->slot, device->function,
                                   PCI_BAR0 + 4 * i));

  out->system_vendor_id = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_SYSTEM_VENDOR_ID);
  out->system_id = ConfigReadWord(device->bus, device->slot, device->function,
                                  PCI_SYSTEM_ID);

  out->expROMaddr =
      combineWord(ConfigReadWord(device->bus, device->slot, device->function,
                                 PCI_EXP_ROM_BASE_ADDR + 2),
                  ConfigReadWord(device->bus, device->slot, device->function,
                                 PCI_EXP_ROM_BASE_ADDR));

  out->capabilitiesPtr =
      exportByte(ConfigReadWord(device->bus, device->slot, device->function,
                                PCI_CAPABILITIES_PTR),
                 true);

  uint32_t interruptLine_interruptPIN = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_INTERRUPT_LINE);
  out->interruptLine = exportByte(interruptLine_interruptPIN, true);
  out->interruptPIN = exportByte(interruptLine_interruptPIN, false);

  uint32_t minGrant_maxLatency = ConfigReadWord(
      device->bus, device->slot, device->function, PCI_MIN_GRANT);
  out->minGrant = exportByte(minGrant_maxLatency, true);
  out->maxLatency = exportByte(minGrant_maxLatency, false);
}

void initiatePCI() {
  for (uint8_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
    for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
        if (!FilterDevice(bus, slot, function))
          continue;

        PCIdevice *device = (PCIdevice *)malloc(sizeof(PCIdevice));
        GetDevice(device, bus, slot, function);
        switch (device->class_id) {
        case PCI_CLASS_CODE_NETWORK_CONTROLLER:
          initiateNIC(device);
          break;
        default:
          break;
        }
        free(device);
      }
    }
  }
}
