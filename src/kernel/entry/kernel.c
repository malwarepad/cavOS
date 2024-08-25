#include <bootloader.h>
#include <console.h>
#include <disk.h>
#include <elf.h>
#include <fakefs.h>
#include <fastSyscall.h>
#include <fb.h>
#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <kb.h>
#include <limine.h>
#include <malloc.h>
#include <md5.h>
#include <mouse.h>
#include <nic_controller.h>
#include <paging.h>
#include <pci.h>
#include <pmm.h>
#include <psf.h>
#include <rtc.h>
#include <serial.h>
#include <shell.h>
#include <string.h>
#include <sys.h>
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

static volatile LIMINE_BASE_REVISION(2);
void _start(void) {
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
  initiateMouse();

  debugf("\n====== REACHED SYSTEM ======\n");
  initiateTimer(1000);
  initiateNetworking();
  initiatePCI();
  firstMountPoint = 0;
  fsMount("/", CONNECTOR_AHCI, 0, 1);
  fsMount("/boot/", CONNECTOR_AHCI, 0, 0);
  fsMount("/dev/", CONNECTOR_DEV, 0, 0);
  fsMount("/sys/", CONNECTOR_SYS, 0, 0);

  // any filesystem operations depend on currentTask
  initiateTasks();

  // just in case there's another font preference
  psfLoadFromFile(DEFAULT_FONT_PATH);

  initiateSyscallInst();
  initiateSyscalls();

  initiateSSE();
  // initiateTasks();

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
