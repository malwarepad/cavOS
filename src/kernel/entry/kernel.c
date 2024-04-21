#include <bootloader.h>
#include <console.h>
#include <disk.h>
#include <elf.h>
#include <fb.h>
#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <kb.h>
#include <limine.h>
#include <malloc.h>
#include <md5.h>
#include <nic_controller.h>
#include <paging.h>
#include <pci.h>
#include <pci_id.h>
#include <pmm.h>
#include <psf.h>
#include <rtc.h>
#include <serial.h>
#include <shell.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <testing.h>
#include <timer.h>
#include <util.h>
#include <vga.h>
#include <vmm.h>

// Kernel entry file
// Copyright (C) 2024 Panagiotis

static volatile LIMINE_BASE_REVISION(1);
int _start(void) {
  systemCallOnProgress = false;
  systemDiskInit = false;

  if (LIMINE_BASE_REVISION_SUPPORTED == false)
    panic();

  initialiseBootloaderParser();
  initiateSerial();

  // Framebuffer doesn't depend on paging, limine prepares it anyways
  initiateVGA();
  initiateConsole();
  clearScreen();

  // None of the two depend on paging
  initiatePMM();
  initiateVMM();

  initiateGDT();
  initiateISR();
  initiatePaging();

  initiateKb();

  debugf("\n====== REACHED SYSTEM ======\n");
  initiateTimer(1000);
  initiateNetworking();
  initiatePCI();
  firstMountPoint = 0;
  fsMount("/", CONNECTOR_AHCI, 0, 0);

  // just in case there's another font preference
  psfLoadFromFile(DEFAULT_FONT_PATH);

  initiatePCI_ID();

  initiateSyscallInst();
  initiateSyscalls();

  initiateSSE();
  initiateTasks();

  testingInit();

  if (!systemDiskInit)
    printf("[warning] System disk has not been detected!\n");

  printf("=========================================\n");
  printf("==     Cave-Like Operating System      ==\n");
  printf("==      Copyright MalwarePad 2024      ==\n");
  printf("=========================================\n\n");

  launch_shell(0);
  panic();
}
