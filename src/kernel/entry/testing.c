#include <arp.h>
#include <elf.h>
#include <fat32.h>
#include <ne2k.h>
#include <pci.h>
#include <task.h>
#include <testing.h>
#include <util.h>
#include <vmm.h>

// Testing stuff
// Copyright (C) 2023 Panagiotis

#define SOME_NETWORKING_STUFF 0
#define PCI_READ 0
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
#if SOME_NETWORKING_STUFF
  // netPacket *packet = (netPacket *)malloc(sizeof(netPacket));
  // int        size = 4096;
  // packet->data = malloc(4096);
  // memset(packet->data, 0, 4096);
  // memset(packet->header.destination_mac, 0, 6);
  // memcpy(packet->header.source_mac, selectedNIC->MAC, 6);
  // packet->header.ethertype = 0;
  // printf("sending...\n");
  // sendNe2000(selectedNIC, packet, size);
  uint8_t thing[4] = {192, 168, 122, 253};
  netArpSend(selectedNIC, thing);
#endif
#if PCI_READ
  for (uint8_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
    for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
        if (!FilterDevice(bus, slot, function))
          continue;

        PCIdevice        *device = (PCIdevice *)malloc(sizeof(PCIdevice));
        PCIgeneralDevice *out =
            (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
        GetDevice(device, bus, slot, function);
        GetGeneralDevice(device, out);

        debugf("portBase: 0x%X\n"
               "bus: 0x%X\n"
               "slot: 0x%X\n"
               "function: 0x%X\n"
               "vendor_id: 0x%X\n"
               "device_id: 0x%X\n"
               "command: 0x%X\n"
               "status: 0x%X\n"
               "revision: 0x%X\n"
               "progIF: 0x%X\n"
               "class_id: 0x%X\n"
               "subclass_id: 0x%X\n"
               "cacheLineSize: 0x%X\n"
               "latencyTimer: 0x%X\n"
               "headerType: 0x%X\n"
               "bist: 0x%X\nbar0:0x%X\nExpROM: 0x%X\nEND DEVICE!!\n\n",
               device->portBase, device->bus, device->slot, device->function,
               device->vendor_id, device->device_id, device->command,
               device->status, device->revision, device->progIF,
               device->class_id, device->subclass_id, device->cacheLineSize,
               device->latencyTimer, device->headerType, device->bist,
               out->bar[0], out->expROMaddr);

        free(device);
        free(out);
      }
    }
  }
#endif

  // elf_execute("/main.cav");

  // FAT32_Directory *dir = (FAT32_Directory *)malloc(sizeof(FAT32_Directory));
  // openFile(dir, "/files/lorem.txt");
  // char *out = (char *)malloc(dir->filesize);
  // debugf("first thing: low: %x high: %x\n", dir->firstClusterLow,
  //        dir->firstClusterHigh);
  // readFileContents(&out, dir);
  // debugf("%s\n", out);
  // free(out);
  // free(dir);

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