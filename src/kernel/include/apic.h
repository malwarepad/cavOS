#include "pci.h"
#include "types.h"

#ifndef APIC_H
#define APIC_H

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define APIC_REGISTER_ID 0x20
#define APIC_REGISTER_APICID 0x20
#define APIC_REGISTER_EOI 0x0B0
#define APIC_REGISTER_SPURIOUS 0x0F0
#define APIC_REGISTER_LVT_TIMER 0x320
#define APIC_REGISTER_TIMER_INITCNT 0x380
#define APIC_REGISTER_TIMER_CURRCNT 0x390
#define APIC_REGISTER_TIMER_DIV 0x3E0

#define APIC_LVT_TIMER_MODE_PERIODIC (1 << 17)

// APIC quick access
// (same address for different cores)
uint64_t apicPhys;
uint64_t apicVirt;

// I/O APIC quick access
typedef struct IOAPIC {
  struct IOAPIC *next;
  uint8_t        id;

  uint64_t ioapicPhys;
  uint64_t ioapicVirt;

  int ioapicRedStart;
  int ioapicRedEnd; // NOT max!
} IOAPIC;
IOAPIC *firstIoapic;

// no way to deallocate vectors, should be good enough
#define MAX_IRQ 256
uint8_t *irqPerCpu;
uint8_t  irqGenericArray[MAX_IRQ];
uint32_t lapicGenericArray[MAX_IRQ];

void initiateAPIC();
void smpInitiateAPIC();

uint8_t ioApicRedirect(uint8_t irq, bool ignored);
uint8_t ioApicPciRegister(PCIdevice *device, PCIgeneralDevice *details);
uint8_t irqPerCoreAllocate(uint8_t gsi, uint32_t *lapicId);

uint32_t apicRead(uint32_t offset);
void     apicWrite(uint32_t offset, uint32_t value);

uint32_t apicCurrentCore();

#endif
