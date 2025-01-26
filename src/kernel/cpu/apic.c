#include <acpi.h>
#include <apic.h>
#include <bootloader.h>
#include <linked_list.h>
#include <malloc.h>
#include <system.h>
#include <timer.h>

// Advanced Programmable Interrupt Controller driver
// (fancy way to say SMP-friendly 8259 PIC)
// Copyright (C) 2024 Panagiotis

/*
 * Trigger mode: 0 is edge-triggered, 1 is level-triggered.
 * Pin polarity: 0 is active-high, 1 is active-low.
 */

bool apicCheck() {
  uint32_t eax = 1; // info
  uint32_t ebx, ecx, edx;

  cpuid(&eax, &ebx, &ecx, &edx);

  // bit 9 in edx is APIC support
  return (edx & (1 << 9)) != 0;
}

/* APIC */

void apicWrite(uint32_t offset, uint32_t value) {
  uint32_t volatile *ptr = (uint32_t volatile *)(apicVirt + offset);
  *ptr = value;
}

uint32_t apicRead(uint32_t offset) {
  uint32_t volatile *ptr = (uint32_t volatile *)(apicVirt + offset);
  return *ptr;
}

/* I/O APIC */

uint32_t ioApicRead(uint64_t ioapicVirt, uint32_t reg) {
  uint32_t volatile *ioapic = (uint32_t volatile *)ioapicVirt;
  ioapic[0] = (reg & 0xff);
  return ioapic[4];
}

void ioApicWrite(uint64_t ioapicVirt, uint32_t reg, uint32_t value) {
  uint32_t volatile *ioapic = (uint32_t volatile *)ioapicVirt;
  ioapic[0] = (reg & 0xff);
  ioapic[4] = value;
}

void ioApicWriteRedEntry(uint64_t ioApicVirt, uint8_t entry, uint8_t vector,
                         uint8_t delivery, uint8_t destmode, uint8_t polarity,
                         uint8_t mode, uint8_t mask, uint8_t dest) {
  uint32_t val = vector;
  val |= (delivery & 0b111) << 8;
  val |= (destmode & 1) << 11;
  val |= (polarity & 1) << 13;
  val |= (mode & 1) << 15;
  val |= (mask & 1) << 16;

  ioApicWrite(ioApicVirt, 0x10 + entry * 2, val);
  ioApicWrite(ioApicVirt, 0x11 + entry * 2, (uint32_t)dest << 24);
}

IOAPIC *ioApicFetch(uint8_t irq) {
  IOAPIC *browse = firstIoapic;
  while (browse) {
    // = cause of +1
    if (irq >= browse->ioapicRedStart && irq <= browse->ioapicRedEnd)
      break;
    browse = browse->next;
  }
  return browse;
}

/* Base stuff */

size_t apicGetBase() { return rdmsr(IA32_APIC_BASE_MSR) & 0xFFFFF000; }
void   apicSetBase(size_t apic) {
  wrmsr(IA32_APIC_BASE_MSR,
          apic | IA32_APIC_BASE_MSR_ENABLE | IA32_APIC_BASE_MSR_BSP);
}

/* PCI routing */

uacpi_iteration_decision uacpiBusMatch(void *user, uacpi_namespace_node *node,
                                       uint32_t _maxDepth) {
  size_t  *udata = (size_t *)user;
  uint64_t currBus = 0;

  // _BBN = PCI bus number
  uacpi_status status = uacpi_eval_simple_integer(node, "_BBN", &currBus);
  if (uacpi_unlikely(status == UACPI_STATUS_NOT_FOUND))
    currBus = 0;
  // else
  //   return UACPI_ITERATION_DECISION_CONTINUE;

  // debugf("{%ld==%ld} ", currBus, udata[0]);
  if (udata[0] != currBus)
    return UACPI_ITERATION_DECISION_CONTINUE;

  // found!
  *((size_t *)udata[1]) = (size_t)node;
  return UACPI_ITERATION_DECISION_BREAK;
}

uint8_t ioApicPciRegister(PCIdevice *device, PCIgeneralDevice *details) {
  uacpi_namespace_node *busNamespace = 0;
  uint8_t               ret = 0;

  // hand off
  size_t udata[2] = {0, (size_t)&busNamespace};
  uacpi_find_devices("PNP0A03", uacpiBusMatch, (void *)udata);

  if (!busNamespace) {
    debugf("[ioapic::pci::redirect] No namespace/bus found!\n");
    return 0;
  }

  // get table
  uacpi_pci_routing_table *pciRoutingTable = 0;
  if (uacpi_get_pci_routing_table(busNamespace, &pciRoutingTable) !=
      UACPI_STATUS_OK) {
    debugf("[ioapic::pci::redirect] No PCI routing table found!\n");
    return 0;
  }

  // everything will be compared against these
  uint8_t bus = device->bus;
  uint8_t slot = device->slot;
  uint8_t function = device->function;
  uint8_t pin = details->interruptPIN;

  if (bus != 0) {
    // not on primary bus! start investigating..
    if (!GetParentBridge(0, &bus, &slot, &function, device->bus)) {
      debugf("[ioapic::pci::redirect] Couldn't find parent bridge! bus{%d}\n",
             device->bus);
      return 0;
    }
    pin = (device->slot + pin) % 4;
  }

  int      trigger = 1;
  int      polarity = 1;
  uint32_t gsi = 0;

  for (size_t i = 0; i < pciRoutingTable->num_entries; i++) {
    uacpi_pci_routing_table_entry *entry = &pciRoutingTable->entries[i];
    if (entry->pin != (pin - 1))
      continue;

    uint16_t currFunction = (entry->address & 0xffff);
    uint16_t currSlot = (entry->address >> 16);

    // debugf("[f:%d:%d s:%d:%d] ", currFunction, function, currSlot, slot);

    if (currSlot != slot ||
        (currFunction != function && currFunction != 0xFFFF))
      continue;

    if (entry->source == 0) {
      // found!
      // debugf("from the !\n");
      gsi = entry->index;
      break;
    }

    // investigate
    uacpi_resources *resources = 0;
    if (uacpi_get_current_resources(entry->source, &resources) !=
        UACPI_STATUS_OK) {
      debugf("[apic] Couldn't fetch resources! source{%d}\n", entry->source);
      goto cleanup;
    }
    if (entry->index > (resources->length - 1)) {
      debugf("[apic] Almost overflowed during lookup! index{%d} len{%d}\n",
             entry->index, resources->length);
      goto cleanup;
    }

    uacpi_resource *resource = &resources->entries[entry->index];
    switch (resource->type) {
    case UACPI_RESOURCE_TYPE_IRQ:
      gsi = resource->irq.irqs[0];
      polarity = resource->irq.polarity == UACPI_POLARITY_ACTIVE_LOW ? 1 : 0;
      trigger = resource->irq.triggering == UACPI_TRIGGERING_EDGE ? 0 : 1;
      break;
    case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
      gsi = resource->extended_irq.irqs[0];
      polarity =
          resource->extended_irq.polarity == UACPI_POLARITY_ACTIVE_LOW ? 1 : 0;
      trigger =
          resource->extended_irq.triggering == UACPI_TRIGGERING_EDGE ? 0 : 1;
      break;
    default:
      debugf("[apic] Weird resource type! type{%d}\n", resource->type);
      panic();
      break;
    }

    uacpi_free_resources(resources);
    break;
  }

  debugf("[ioapic::pci] Redirection [%02d:%02d.%d] -> [%02d:%02d.%d]: gsi{%d} "
         "[%d:%d]\n",
         device->bus, device->slot, device->function, bus, slot, function, gsi,
         polarity, trigger);
  if (!gsi) {
    debugf("[ioapic::pci::redirect] No gsi found!\n");
    goto cleanup;
  }

  IOAPIC *ioapic = ioApicFetch(gsi);
  if (!ioapic) {
    debugf("[apic] FATAL! Couldn't find I/O APIC for gsi{%d}!\n", gsi);
    panic();
  }

  uint8_t  entry = gsi - ioapic->ioapicRedStart;
  uint32_t lapic = 0;
  uint8_t  targIrq = irqPerCoreAllocate(gsi, &lapic);
  ioApicWriteRedEntry(ioapic->ioapicVirt, entry, targIrq, 0, 0, polarity,
                      trigger, false, lapic);
  ret = targIrq;

cleanup:
  uacpi_free_pci_routing_table(pciRoutingTable);
  return ret;
}

/* Legacy ISA redirection */

uint8_t ioApicRedirect(uint8_t irq, bool ignored) {
  uint8_t polarity = 0; // active high
  uint8_t trigger = 0;  // edge triggered

  // browse the override table
  size_t curr = (size_t)madt + sizeof(struct acpi_madt);
  size_t end = curr + madt->hdr.length;
  while (curr < end) {
    struct acpi_entry_hdr *browse = (struct acpi_entry_hdr *)curr;
    if (browse->type == ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
      struct acpi_madt_interrupt_source_override *specialized =
          (struct acpi_madt_interrupt_source_override *)browse;
      if (specialized->source == irq) {
        // found!
        irq = specialized->gsi;
        polarity = (specialized->flags & 2) ? 1 : 0;
        trigger = (specialized->flags & 8) ? 1 : 0;
        break;
      }
    }
    curr += browse->length;
  }

  IOAPIC *ioapic = ioApicFetch(irq);
  if (!ioapic) {
    debugf("[apic] FATAL! Couldn't find I/O APIC for irq{%d}!\n", irq);
    panic();
  }

  // aloc
  uint32_t lapic = 0;
  uint8_t  targIrq = irqPerCoreAllocate(irq, &lapic);

  uint8_t entry = irq - ioapic->ioapicRedStart;
  ioApicWriteRedEntry(ioapic->ioapicVirt, entry, targIrq, 0, 0, polarity,
                      trigger, ignored, lapic);

  return targIrq;
}

/* IRQ/Vector allocation utilities */
int  irqLast = 0; // starting point
void initiateIrqPerCore() {
  irqPerCpu = calloc(sizeof(uint8_t) * bootloader.smp->cpu_count, 1);
}

uint8_t irqPerCoreAllocate(uint8_t gsi, uint32_t *lapicId) {
  // check for existing items
  for (int i = 0; i < irqLast; i++) {
    if (irqGenericArray[i] == gsi) {
      // found smth for this gsi
      *lapicId = lapicGenericArray[i];
      return 32 + i;
    }
  }

  // spurious vector checks (extendable, hence the while)
  while (irqLast == 0xff)
    irqLast++;

  // check boundaries
  int irqGenericIndex = irqLast;
  if (irqGenericIndex > (MAX_IRQ - 1)) {
    debugf("[irq] Irq overflow! index{%d}\n", irqGenericArray);
    panic();
  }

  // find the core with the least irqs allocated
  int    min = MAX_IRQ;
  size_t minIndex = 0;
  for (size_t i = 0; i < 1; i++) { // todo: bootloader.smp->cpu_count
    if (irqPerCpu[i] < min) {
      irqPerCpu[i] = min;
      minIndex = i;
    }
  }

  irqGenericArray[irqGenericIndex] = gsi; // keep track
  lapicGenericArray[irqGenericIndex] =
      bootloader.smp->cpus[minIndex]->lapic_id; // keep track
  irqPerCpu[minIndex]++;                        // count per CPU

  irqLast++; // for the next one
  *lapicId = bootloader.smp->cpus[minIndex]->lapic_id;
  return irqGenericIndex + 32;
}

void initiateAPIC() {
  if (!apicCheck()) {
    debugf("[apic] FATAL! APIC is not supported!\n");
    panic();
  }

  initiateIrqPerCore(); // structure ready from the start
  apicPhys = apicGetBase();

  // parse acpi tables
  uacpi_table  madtTbl;
  uacpi_status madtOperation = uacpi_table_find_by_signature("APIC", &madtTbl);
  if (madtOperation != UACPI_STATUS_OK) {
    debugf("[apic] Couldn't find MADT table: %s\n",
           uacpi_status_to_string(madtOperation));
    panic();
  }

  madt = madtTbl.ptr;
  if (madt->local_interrupt_controller_address != apicPhys) {
    debugf("[apic] Warning! MADT physical address doesn't match fetched one! "
           "madt_phys{%lx} rdmsr_phys{%lx}\n",
           madt->local_interrupt_controller_address, apicPhys);
    apicPhys = madt->local_interrupt_controller_address;
  }

  // the acpi list itself
  size_t curr = (size_t)madt + sizeof(struct acpi_madt);
  size_t end = curr + madt->hdr.length;
  int    ioapics = 0;
  while (curr < end) {
    struct acpi_entry_hdr *browse = (struct acpi_entry_hdr *)curr;
    if (browse->type == ACPI_MADT_ENTRY_TYPE_IOAPIC) {
      struct acpi_madt_ioapic *specialized = (struct acpi_madt_ioapic *)browse;
      IOAPIC                  *ioapic =
          (IOAPIC *)LinkedListAllocate((void **)&firstIoapic, sizeof(IOAPIC));
      ioapic->id = specialized->id;
      ioapic->ioapicPhys = specialized->address;
      ioapic->ioapicVirt = bootloader.hhdmOffset + ioapic->ioapicPhys;
      ioapic->ioapicRedStart = specialized->gsi_base;
      int capacity = (ioApicRead(ioapic->ioapicVirt, 1) >> 16) & 0xFF;
      ioapic->ioapicRedEnd = ioapic->ioapicRedStart + capacity;
      ioapics++;
    } else if (browse->type == ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE) {
      struct acpi_madt_lapic_address_override *specialized =
          (struct acpi_madt_lapic_address_override *)browse;
      debugf("[apic] Warning! Local APIC override! old{%lx} new{%lx}\n",
             apicPhys, specialized->address);
      apicPhys = specialized->address;
    }
    curr += browse->length;
  }

  if (!ioapics) {
    debugf("[apic] No I/O APICs found!\n");
    panic();
  }

  // apicVirt has to be set here due to potential overrides
  apicVirt = bootloader.hhdmOffset + apicPhys;
  debugf("[apic] Detection completed: lapic{%lx} ", apicPhys);
  IOAPIC *browse = firstIoapic;
  while (browse) {
    debugf("ioapic{%lx} ", browse->ioapicPhys);
    browse = browse->next;
  }
  debugf("\n");

  // enable lapic (for the bootstrap core)
  apicSetBase(apicPhys);
  apicWrite(0xF0, apicRead(0xF0) | 0x1FF);
}
