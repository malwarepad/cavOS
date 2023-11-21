#include "../include/testing.h"
#include "../include/elf.h"
#include "../include/task.h"
#include "../include/vmm.h"

// Testing stuff
// Copyright (C) 2023 Panagiotis

#define MEMORY_DETECTION_DRAFT 0
#define FAT32_READ_TEST 0
#define FAT32_DELETION_TEST 0
#define FAT32_ALLOC_STRESS_TEST 0
#define MULTITASKING_PROCESS_TESTING 0
#define VGA_DRAW_TEST 0
#define VGA_FRAMERATE 0

#if MULTITASKING_PROCESS_TESTING
void task1() {
  for (int i = 0; i < 8; i++) {
    printf("task 1: aaa\n");
  }
  asm volatile("mov $1, %eax \n"
               "int $0x80");
}

void task2() {
  for (int i = 0; i < 8; i++) {
    printf("task 2: 1111111111111111111111111111111111111\n");
  }
  asm volatile("mov $1, %eax \n"
               "int $0x80");
}

void task3() {
  for (int i = 0; i < 8; i++) {
    printf("task 3: 423432423\n");
  }
  asm volatile("mov $1, %eax \n"
               "int $0x80");
}
#endif

void testingInit() {
  // elf_execute("/main.cav");
#if VGA_DRAW_TEST
  drawCircle(200, 300, 100, 255, 0, 0);
  drawCircle(400, 300, 100, 0, 255, 0);
  drawCircle(600, 300, 100, 0, 0, 255);
  drawCircle(800, 300, 100, 0, 0, 0);

  asm("cli");
  panic();
#endif

#if VGA_FRAMERATE
  while (1) {
    int randomX = inrand(0, 1024);
    int randomY = inrand(0, 768);
    int random = inrand(0, 600);
    drawRect(randomX, randomY, randomX, randomY, random / 2, random % 255,
             random / 4);
  }
#endif
#if MULTITASKING_PROCESS_TESTING
  create_task(1, (uint32_t)task1, true, PageDirectoryAllocate());
  create_task(2, (uint32_t)task2, true, PageDirectoryAllocate());
  create_task(3, (uint32_t)task3, true, PageDirectoryAllocate());
#endif
#if FAT32_ALLOC_STRESS_TEST
  for (int i = 0; i < 64; i++) {
    FAT32_Directory *dir = (FAT32_Directory *)malloc(sizeof(FAT32_Directory));
    openFile(dir, "/lorem.txt");
    char *out = (char *)malloc(dir->filesize);
    readFileContents(&out, dir);
    debugf("%s\n", out);
    free(out);
    free(dir);

    // sleep(1000);
  }
#endif
#if FAT32_DELETION_TEST
  deleteFile("/files/untitled.txt");
#endif

#if FAT32_READ_TEST
  FAT32_Directory dir;
  if (!openFile(&dir, "/ab.txt"))
    printf("No such file can be found!\n");

  char *contents = readFileContents(&dir);
  printf("%s\n", contents);

  // fileReaderTest();
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
    debugf("\n[%x %x - %x %x] {%x}", mmmt->addr_high, mmmt->addr_low,
           mmmt->len_high, mmmt->len_low, mmmt->type);
  }
  printf("\n");
#endif

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
}