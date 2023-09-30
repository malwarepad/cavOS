#include "../../include/allocation.h"
#include "../../include/ata.h"
#include "../../include/disk.h"
#include "../../include/fat32.h"
#include "../../include/idt.h"
#include "../../include/isr.h"
#include "../../include/kb.h"
#include "../../include/multiboot.h"
#include "../../include/rtc.h"
#include "../../include/shell.h"
#include "../../include/util.h"
#include "../../include/vga.h"

#include <stdint.h>

// Kernel entry file
// Copyright (C) 2023 Panagiotis

#define MEMORY_DETECTION_DRAFT 0
#define FAT32_READ_TEST 0

int kmain(uint32 magic, multiboot_info_t *mbi) {
  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    printf("invalid magic number!");
    asm("hlt");
  }

  /* Check bit 6 to see if we have a valid memory map */
  if (!(mbi->flags >> 6 & 0x1)) {
    printf("invalid memory map given by GRUB bootloader");
    asm("hlt");
  }

  clearScreen();

  string center = "                   ";
  printf("%s=========================================%s\n", center, center);
  printf("%s==     Cave-Like Operating System      ==%s\n", center, center);
  printf("%s==      Copyright MalwarePad 2023      ==%s\n", center, center);
  printf("%s=========================================%s\n\n", center, center);

  isr_install();
  init_memory(mbi);
  initiateFat32();
#if FAT32_READ_TEST
  test();
#endif
#if MEMORY_DETECTION_DRAFT
  printf("[+] Memory detection:");
#endif

#if MEMORY_DETECTION_DRAFT
  /* Loop through the memory map and display the values */
  for (int i = 0; i < mbi->mmap_length; i += sizeof(multiboot_memory_map_t)) {
    multiboot_memory_map_t *mmmt =
        (multiboot_memory_map_t *)(mbi->mmap_addr + i);
    // if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
    printf("\n    [+] Start Addr: %lx | Length: %lx | Size: "
           "%x | Type: %x",
           mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
  }
  printf("\n");
#endif

  printf("\n");

  // findFile("/BOOT       /GRUB       /KERNEL  BIN");
  // findFile("/UNTITLEDLOL/UNTITLEDTXT");
  // findFile("/UNTITLEDTXT");
  // string out;
  // int    ret = followConventionalDirectoryLoop(out, "/hahalol/yes", 2);
  // printf("[%d] %d %s \n", ret, strlength(out), out);

  // printf("\nWelcome to cavOS! The OS that reminds you of how good computers
  // \nwere back then.. Anyway, just execute any command you want\n'help' is
  // your friend :)\n\nNote that this program comes with ABSOLUTELY NO
  // WARRANTY.\n");

  // string str = formatToShort8_3Format("grt.txt");
  // printf("%s [%d]", str, strlength(str));

  // ! this is my testing land lol

  /*printf("writing 0...\r\n");
  char bwrite[512];
  for(i = 0; i < 512; i++)
  {
      bwrite[i] = 0x0;
  }
  write_sectors_ATA_PIO(0x0, 2, bwrite);


  printf("reading...\r\n");
  read_sectors_ATA_PIO(target, 0x0, 1);

  i = 0;
  while(i < 128)
  {
      printf("%x ", target[i] & 0xFF);
      printf("%x ", (target[i] >> 8) & 0xFF);
      i++;
  }
  printf("\n");*/

  launch_shell(0, mbi);
}
