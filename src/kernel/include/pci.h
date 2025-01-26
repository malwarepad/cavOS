#include "isr.h"
#include "types.h"

#ifndef PCI_H
#define PCI_H

// Limits
#define PCI_MAX_BUSES 256
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTIONS 8

// Generic addresses
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// PCI types
typedef enum PCI_DEVICES {
  PCI_DEVICE_GENERAL = 0x0,
  PCI_DEVICE_BRIDGE = 0x1,
  PCI_DEVICE_CARDBUS = 0x2,
} PCI_DEVICES;

// PCI config header
#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0a
#define PCI_CLASS 0x0b
#define PCI_CACHE_LINE_SIZE 0x0c
#define PCI_LATENCY_TIMER 0x0d
#define PCI_HEADER_TYPE 0x0e
#define PCI_BIST 0x0f
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_SECONDARY_BUS 0x09
#define PCI_SYSTEM_VENDOR_ID 0x2C
#define PCI_SYSTEM_ID 0x2E
#define PCI_EXP_ROM_BASE_ADDR 0x30
#define PCI_CAPABILITIES_PTR 0x34
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_MIN_GRANT 0x3E
// PCI Class codes
#define PCI_CLASS_CODE_UNCLASSIFIED 0x0
#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER 0x1
#define PCI_CLASS_CODE_NETWORK_CONTROLLER 0x2
#define PCI_CLASS_CODE_DISPLAY_CONTROLLER 0x3
#define PCI_CLASS_CODE_MULTIMEDIA_CONTROLLER 0x4
#define PCI_CLASS_CODE_MEMORY_CONTROLLER 0x5
#define PCI_CLASS_CODE_BRIDGE_CONTROLLER 0x6
#define PCI_CLASS_CODE_SIMPLE_COMM_CONTROLLER 0x7
#define PCI_CLASS_CODE_BASE_SYSTEM_PERIPHERAL 0x8
#define PCI_CLASS_CODE_INPUT_DEVICE_CONTROLLER 0x9
#define PCI_CLASS_CODE_DOCKING_SYSTEM 0xA
#define PCI_CLASS_CODE_PROCESSOR 0xB
#define PCI_CLASS_CODE_SERIAL_BUS_CONTROLLER 0xC
#define PCI_CLASS_CODE_WIRELESS_CONTROLLER 0xD
#define PCI_CLASS_CODE_INTELLIGENT_CONTROLLER 0xE
#define PCI_CLASS_CODE_SATELLITE_COMM_CONTROLLER 0xF
#define PCI_CLASS_CODE_ENCRYPTION_CONTROLLER 0x10
#define PCI_CLASS_CODE_SIGNAL_PROCESSING_CONTROLLER 0x11
#define PCI_CLASS_CODE_PROCESSING_ACCELERATOR 0x12
#define PCI_CLASS_CODE_NON_ESSENTIAL_INSTRUMENTATION 0x13
#define PCI_CLASS_CODE_COPROCESSOR 0x40
#define PCI_CLASS_CODE_UNASSIGNED 0xFF

#define PCI_PRIMARY_BUS_NUM 0x18
#define PCI_SECONDARY_BUS_NUM 0x19
#define PCI_SUBORDINATE_BUS_NUM 0x20

typedef struct PCIdevice {
  uint16_t bus;
  uint16_t slot;
  uint16_t function;

  uint16_t vendor_id;
  uint16_t device_id;

  uint16_t command;
  uint16_t status;

  uint8_t revision;
  uint8_t progIF; // programming interface
  uint8_t subclass_id;
  uint8_t class_id;

  uint8_t cacheLineSize;
  uint8_t latencyTimer;
  uint8_t headerType;
  uint8_t bist;
} PCIdevice;

typedef struct PCIgeneralDevice {
  uint32_t bar[6];

  uint32_t cardBusCISPtr;

  uint16_t system_id;
  uint16_t system_vendor_id;

  uint32_t expROMaddr;

  // 24 bits (3 bytes) reserved
  uint8_t capabilitiesPtr;

  // 32 bits (4 bytes) reserved

  uint8_t interruptLine;
  uint8_t interruptPIN;
  uint8_t minGrant;
  uint8_t maxLatency;
} PCIgeneralDevice;

typedef enum PCI_DRIVER {
  PCI_DRIVER_NULL = 0,
  PCI_DRIVER_AHCI,
  PCI_DRIVER_RTL8139,
  PCI_DRIVER_RTL8169,
} PCI_DRIVER;

typedef enum PCI_DRIVER_CATEGORY {
  PCI_DRIVER_CATEGORY_NULL = 0,
  PCI_DRIVER_CATEGORY_STORAGE,
  PCI_DRIVER_CATEGORY_NIC,
} PCI_DRIVER_CATEGORY;

typedef struct PCI PCI;
struct PCI {
  PCI *next;

  uint8_t  bus, slot, function;
  uint16_t vendor_id, device_id;

  char               *name;
  PCI_DRIVER          driver;
  PCI_DRIVER_CATEGORY category;

  void       *extra;
  irqHandler *irqHandler;
};
PCI *firstPCI;

#define EXPORT_BYTE(target, first)                                             \
  ((first) ? ((target) & ~0xFF00) : (((target) & ~0x00FF) >> 8))
#define COMBINE_WORD(msb, lsb) (((uint32_t)(msb) << 16) | (lsb))

void initiatePCI();
int  FilterDevice(uint8_t bus, uint8_t slot, uint8_t function);
void GetDevice(PCIdevice *device, uint8_t bus, uint8_t slot, uint8_t function);
void GetGeneralDevice(PCIdevice *device, PCIgeneralDevice *out);
void ConfigWriteDword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                      uint32_t conf);
uint16_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                        uint8_t offset);

PCI *lookupPCIdevice(PCIdevice *device);
void setupPCIdeviceDriver(PCI *pci, PCI_DRIVER driver,
                          PCI_DRIVER_CATEGORY category);

bool GetParentBridge(uint8_t bus, uint8_t *Tbus, uint8_t *Tslot,
                     uint8_t *Tfunction, uint8_t targetBus);

#endif
