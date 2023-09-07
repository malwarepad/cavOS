#include "../../include/allocation.h"
#include "../../include/ata.h"
#include "../../include/disk.h"
#include "../../include/fat32.h"
#include "../../include/idt.h"
#include "../../include/isr.h"
#include "../../include/kb.h"
#include "../../include/multiboot.h"
#include "../../include/shell.h"
#include "../../include/util.h"
#include "../../include/vga.h"

#include <stdint.h>

// Kernel entry file
// Copyright (C) 2023 Panagiotis

int kmain(uint32 magic, multiboot_info_t *mbi) {

  clearScreen();

  string center = "                   ";
  printf("%s=========================================%s\n", center, center);
  printf("%s==     Cave-Like Operating System      ==%s\n", center, center);
  printf("%s==      Copyright MalwarePad 2023      ==%s\n", center, center);
  printf("%s=========================================%s\n\n", center, center);

  isr_install();
  initiateFat32();
  printf("\n");

  // Initialize memory management based on Multiboot info
  init_memory(mbi);

  // Example usage
  int *my_int = (int *)malloc(sizeof(int));
  if (my_int != NULL) {
    *my_int = 42;
  }

  // Free allocated memory
  if (my_int != NULL) {
    free(my_int);
  }

  // printf("\nWelcome to cavOS! The OS that reminds you of how good computers
  // \nwere back then.. Anyway, just execute any command you want\n'help' is
  // your friend :)\n\nNote that this program comes with ABSOLUTELY NO
  // WARRANTY.\n");

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
