#include "limine.h"
#include "types.h"

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

// From the linker
extern uint64_t kernel_text_start, kernel_text_end;
extern uint64_t kernel_rodata_start, kernel_rodata_end;
extern uint64_t kernel_data_start, kernel_data_end;
extern uint64_t kernel_start, kernel_end;

typedef struct Bootloader {
  size_t hhdmOffset;
  size_t kernelVirtBase;
  size_t kernelPhysBase;

  size_t rsdp;

  size_t   mmTotal;
  uint64_t mmEntryCnt;
  LIMINE_PTR(struct limine_memmap_entry **) mmEntries;
  LIMINE_PTR(struct limine_smp_response *) smp;
  uint64_t smpBspIndex;
} Bootloader;

Bootloader bootloader;

void initialiseBootloaderParser();

#endif