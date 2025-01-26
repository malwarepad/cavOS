#include <bootloader.h>
#include <limine.h>
#include <system.h>

// Parses information off our bootloader (limine)
// Copyright (C) 2024 Panagiotis

static volatile struct limine_paging_mode_request liminePagingreq = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL};

static volatile struct limine_kernel_address_request limineKrnreq = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

static volatile struct limine_hhdm_request limineHHDMreq = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

static volatile struct limine_memmap_request limineMMreq = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

static volatile struct limine_smp_request limineSmpReq = {
    .id = LIMINE_SMP_REQUEST, .revision = 0};

static volatile struct limine_rsdp_request limineRsdpReq = {
    .id = LIMINE_RSDP_REQUEST, .revision = 0};

void initialiseBootloaderParser() {
  // Paging mode
  struct limine_paging_mode_response *liminePagingres =
      liminePagingreq.response;
  if (liminePagingres->mode != LIMINE_PAGING_MODE_X86_64_4LVL) {
    debugf("[paging] We explicitly asked for level 4 paging!\n");
    panic();
  }

  // HHDM (randomized kernel positionings)
  struct limine_hhdm_response *limineHHDMres = limineHHDMreq.response;
  bootloader.hhdmOffset = limineHHDMres->offset;

  // Kernel address
  struct limine_kernel_address_response *limineKrnres = limineKrnreq.response;
  bootloader.kernelVirtBase = limineKrnres->virtual_base;
  bootloader.kernelPhysBase = limineKrnres->physical_base;

  // Memory map
  struct limine_memmap_response *mm_response = limineMMreq.response;
  bootloader.mmEntries = mm_response->entries;
  bootloader.mmEntryCnt = mm_response->entry_count;

  // Total memory
  bootloader.mmTotal = 0;
  for (int i = 0; i < mm_response->entry_count; i++) {
    struct limine_memmap_entry *entry = mm_response->entries[i];
    // entry->type != LIMINE_MEMMAP_FRAMEBUFFER &&
    if (entry->type != LIMINE_MEMMAP_RESERVED)
      bootloader.mmTotal += entry->length;
  }

  // SMP
  struct limine_smp_response *smp_response = limineSmpReq.response;
  bootloader.smp = smp_response;

  // Bootstrap core index
  bootloader.smpBspIndex = (uint64_t)(-1);
  for (int i = 0; i < smp_response->cpu_count; i++) {
    struct limine_smp_info *entry = smp_response->cpus[i];
    if (entry->lapic_id == smp_response->bsp_lapic_id)
      bootloader.smpBspIndex = i;
  }
  if (bootloader.smpBspIndex == (uint64_t)(-1)) {
    debugf("[bootloader] Couldn't find bootstrap core!\n");
    panic();
  }

  // RSDP
  // todo: revision >= 3 and it's not virtual!
  struct limine_rsdp_response *rsdp_response = limineRsdpReq.response;
  bootloader.rsdp = (size_t)rsdp_response->address - bootloader.hhdmOffset;
}
