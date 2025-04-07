#include <bootloader.h>
#include <console.h>
#include <elf.h>
#include <fb.h>
#include <kb.h>
#include <malloc.h>
#include <nic_controller.h>
#include <paging.h>
#include <pci.h>
#include <pmm.h>
#include <rtc.h>
#include <shell.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

#include <lwip/dns.h>

// Temporary kernelspace shell
// Copyright (C) 2024 Panagiotis

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

void fetch() {
  printf("\n      ^         name: cavOS");
  printf("\n     / \\        memory: %ldMB",
         DivRoundUp(DivRoundUp(bootloader.mmTotal, 1024), 1024));
  printf("\n    /   \\       uptime: %lds", DivRoundUp(timerTicks, 1000));
  printf("\n   /  ^  \\  _");
  printf("\n   \\ \\ / / / \\");
  printf("\n           \\_/  \n");
}

void help() {
  printf("\n========================== GENERIC ==========================");
  printf("\n= cmd            : Launch a new recursive Shell             =");
  printf("\n= clear          : Clears the screen                        =");
  printf("\n= echo           : Reprintf a given text                    =");
  printf("\n= exit           : Quits the current shell                  =");
  printf("\n= fetch          : Brings you some system information       =");
  printf("\n= time           : Tells you the time and date from BIOS    =");
  printf("\n= lspci          : Lists PCI device info                    =");
  printf("\n= dump           : Dumps some of the bitmap allocator       =");
  printf("\n= draw           : Tests framebuffer by drawing a rectangle =");
  printf("\n= proctest       : Tests multitasking support               =");
  printf("\n= exec           : Runs a cavOS binary of your choice       =");
  printf("\n= bash           : GNU Bash, your portal to userspace!      =");
  printf("\n=============================================================\n");
  printf("\n========================= FILESYSTEM ========================");
  printf("\n= readfile       : Read a file's contents                   =");
  printf("\n= readdisk       : Tests out the disk reading algorythm     =");
  printf("\n=============================================================\n");
  printf("\n========================= NETWORKING ========================");
  printf("\n= net            : Get info about your NICs (+DHCP)         =");
  printf("\n= arptable       : Output the ARP table                     =");
  printf("\n= arping         : Ask a local IP for it's MAC address      =");
  printf("\n= ping           : Send an ICMP request to an IP            =");
  printf("\n=============================================================\n");
}

void readDisk() {
  // if (!fat->works) {
  //   printf("\nFAT32 was not initalized properly on boot!\n");
  //   return;
  // }

  clearScreen();
  printf("=========================================\n");
  printf("====        cavOS readdisk 1.0       ====\n");
  printf("====    Copyright MalwarePad 2024    ====\n");
  printf("=========================================\n");

  printf("\n1MB Limine sector: LBA=0 Offset=0, FAT32 sector: LBA=2048 "
         "Offset=1048576");
  printf("\nInsert LBA (LBA = Offset / Sector Size): ");

  char *choice = (char *)malloc(200);
  readStr(choice);
  int lba = atoi(choice);
  printf("\n\n");

  // saving memory like scrooge mcduck out here
  memset(choice, 0, 200);
  snprintf(choice, 200, "reading disk{0} LBA{%d}:", lba);

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  hexDump(choice, rawArr, SECTOR_SIZE, 16, printf);

  printf("\n");
  free(rawArr);
  free(choice);
}

void echo(char *ch) {
  printf("\nInsert argument 1: ");
  char str[200];
  readStr(str);
  printf("\n%s\n", str);
}

bool run(char *binary, bool wait, int _argc, char **_argv) {
  char *argv[] = {binary};
  Task *task = 0;
  if (_argc > 0)
    task = elfExecute(binary, _argc, _argv, 0, 0, false);
  else
    task = elfExecute(binary, 1, argv, 0, 0, false);
  if (!task)
    return false;

  task->parent = currentTask;
  int stdin = fsUserOpen(task, "/dev/stdin", O_RDWR | O_APPEND, 0);
  int stdout = fsUserOpen(task, "/dev/stdout", O_RDWR | O_APPEND, 0);
  int stderr = fsUserOpen(task, "/dev/stderr", O_RDWR | O_APPEND, 0);

  if (stdin < 0 || stdout < 0 || stderr < 0) {
    debugf("[elf] Couldn't establish basic IO!\n");
    panic();
  }

  OpenFile *fdStdin = fsUserGetNode(task, stdin);
  OpenFile *fdStdout = fsUserGetNode(task, stdout);
  OpenFile *fdStderr = fsUserGetNode(task, stderr);

  fdStdin->id = 0;
  fdStdout->id = 1;
  fdStderr->id = 2;

  taskCreateFinish(task);

  if (task && wait) {
    currentTask->waitingForPid = task->id;
    currentTask->state = TASK_STATE_WAITING_CHILD_SPECIFIC;
    handControl();
    // while (currentTask->state == TASK_STATE_WAITING_CHILD)
    //   ;
  }

  return true;
}

void launch_shell(int n) {
  debugf("[shell] Kernel-land shell launched: n{%d}\n", n);
  char *ch = (char *)malloc(200);
  char *prompt = "$ ";

  do {
    printf("%s", prompt);
    readStr(ch);
    if (strEql(ch, "cmd")) {
      printf("\nYou are already in cmd. A new recursive shell is opened\n");
      launch_shell(n + 1);
    } else if (strEql(ch, "clear")) {
      clearScreen();
    } else if (strEql(ch, "echo")) {
      echo(ch);
    } else if (strEql(ch, "dump")) {
      printf("\n");
      BitmapDumpBlocks(&physical);
    } else if (strEql(ch, "help")) {
      help();
    } else if (strEql(ch, "readdisk")) {
      readDisk();
    } else if (strEql(ch, "draw")) {
      clearScreen();
      printf("Draw rectangle! width (px): ");
      char *widthStr = (char *)malloc(200);
      readStr(widthStr);
      int width = atoi(widthStr);
      printf(" height (px): ");
      char *heightStr = (char *)malloc(200);
      readStr(heightStr);
      int height = atoi(heightStr);

      drawRect((framebufferWidth / 2) - (width / 2), getConsoleY() + 16, width,
               height, 255, 255, 255);
      setConsoleY(getConsoleY() + height + 16);
      printf("\n");
    } else if (strEql(ch, "drawimg")) {
      printf("\nInput cavimg file's path: ");
      char *buff = (char *)malloc(1024);
      readStr(buff);
      printf("\n");
      OpenFile *dir = fsKernelOpen(buff, O_RDONLY, 0);
      if (!dir) {
        printf("File cannot be found!\n");
        continue;
      }
      char *out = (char *)malloc(fsGetFilesize(dir));
      fsReadFullFile(dir, (uint8_t *)out);
      fsKernelClose(dir);
      clearScreen();

      uint32_t width = 800;
      uint32_t height = 600;
      uint32_t max = width * height;
      for (int i = 0; i < max; ++i) {
        int bufferPos = i * 3;

        int r = out[bufferPos];
        int g = out[bufferPos + 1];
        int b = out[bufferPos + 2];

        int x = i % width;
        int y = i / width;

        drawPixel(x, y, r, g, b);
      }
      setConsoleY(getConsoleY() + height + 16);

      free(out);
      free(buff);
    } else if (strEql(ch, "readfile")) {
      printf("\nInput file: ");
      char *buff = (char *)malloc(1024);
      readStr(buff);
      printf("\n");
      OpenFile *dir = fsKernelOpen(buff, O_RDONLY, 0);
      if (!dir) {
        printf("File cannot be found!\n");
        continue;
      }
      uint32_t filesize = fsGetFilesize(dir);
      char    *out = (char *)malloc(filesize);
      fsReadFullFile(dir, (uint8_t *)out);
      fsKernelClose(dir);
      for (int i = 0; i < filesize; i++) {
        printf("%c", out[i]);
      }
      printf("\n");
    } else if (strEql(ch, "crack")) {
      Task *fr = firstTask->next;
      fr->state = TASK_STATE_READY;
      printf("\n");
    } else if (strEql(ch, "bash")) {
      printf("\n");
      run("/bin/bash", true, 0, 0);
    } else if (strEql(ch, "net")) {
      printf("\nWarning: networking is still very early in testing!\n");
      printf("=========================================\n");
      printf("==       Networking configuration      ==\n");
      printf("==      Copyright MalwarePad 2024      ==\n");
      printf("=========================================\n\n");
      PCI *pci = firstPCI;
      while (pci) {
        if (pci->category != PCI_DRIVER_CATEGORY_NIC) {
          pci = pci->next;
          continue;
        }
        NIC *nic = pci->extra;

        const ip_addr_t *dns = dns_getserver(0);

        uint8_t *ipaddr = (uint8_t *)&nic->lwip.ip_addr.addr;
        uint8_t *subnetaddr = (uint8_t *)&nic->lwip.netmask.addr;
        uint8_t *serveraddr = (uint8_t *)&nic->lwip.gw;
        uint8_t *dnsaddr = (uint8_t *)&dns[0].addr;

        if (nic == selectedNIC)
          printf("[%d]: ", nic->type);
        else
          printf("%d: ", nic->type);
        printf("%02X:%02X:%02X:%02X:%02X:%02X IPv4{%d.%d.%d.%d} "
               "SubnetMask{%d.%d.%d.%d} Router{%d.%d.%d.%d} DNS{%d.%d.%d.%d}\n",
               nic->MAC[0], nic->MAC[1], nic->MAC[2], nic->MAC[3], nic->MAC[4],
               nic->MAC[5], ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3],
               subnetaddr[0], subnetaddr[1], subnetaddr[2], subnetaddr[3],
               serveraddr[0], serveraddr[1], serveraddr[2], serveraddr[3],
               dnsaddr[0], dnsaddr[1], dnsaddr[2], dnsaddr[3]);
        pci = pci->next;
      }
    } else if (strEql(ch, "lspci_")) {
      printf("\n");
      for (uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
          for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
            if (!FilterDevice(bus, slot, function))
              continue;

            PCIdevice        *device = (PCIdevice *)malloc(sizeof(PCIdevice));
            PCIgeneralDevice *out =
                (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
            GetDevice(device, bus, slot, function);
            GetGeneralDevice(device, out);
            printf("[%d:%d:%d] vendor=%x device=%x class_id=%x subclass_id:%x "
                   "system_vendor_id=%x system_id=%x\n",
                   bus, slot, function, device->vendor_id, device->device_id,
                   device->class_id, device->subclass_id, out->system_vendor_id,
                   out->system_id);
            free(device);
            free(out);
          }
        }
      }
    } else if (strEql(ch, "time")) {
      // BitmapDumpBlocks();
      RTC *rtc = (RTC *)malloc(sizeof(RTC));
      readFromCMOS(rtc);
      printf("\n%02d:%02d:%02d %02d/%02d/%04d\n", rtc->hour, rtc->minute,
             rtc->second, rtc->day, rtc->month, rtc->year);
      free(rtc);
    } else if (strEql(ch, "exec")) {
      printf("\nFile path to executable: ");
      char *filepath = (char *)malloc(200);
      memset(filepath, 0, 200);
      readStr(filepath);
      printf("\n");

      bool ret = run(filepath, true, 0, 0);
      if (!ret) {
        printf("Failure executing %s!\n", filepath);
        continue;
      }

      free(filepath);
    } else if (strEql(ch, "tasks")) {
      printf("\n");
      Task *browse = firstTask;
      while (browse) {
        printf("%ld: [%c] heap{0x%016lx-0x%016lX}\n", browse->id,
               browse->kernel_task ? '-' : 'u', browse->heap_start,
               browse->heap_end);

        browse = browse->next;
      }
    } else if (strEql(ch, "lspci")) {
      printf("\n");
      PCI *browse = firstPCI;
      while (browse) {
        printf("[%d:%d:%d] %s\n", browse->bus, browse->slot, browse->function,
               browse->name);

        browse = browse->next;
      }
    } else if (strEql(ch, "proctest")) {
      printf("\n");
      for (int i = 0; i < 4; i++)
        run("/usr/bin/testing", false, 0, 0);
    } else if (strEql(ch, "cwm")) {
      printf("\n%s\n",
             "After taking some time off the project, I realized I was "
             "putting my time and resources on the wrong places... From "
             "now on, I will perfect the basics before trying to create "
             "something that requires them! Part of that is the window "
             "manager (cwm) which I will temporarily remove from the "
             "operating system.");
    } else if (strEql(ch, "about")) {
      printf("\n");
      printf("=========================================\n");
      printf("==        cavOS kernel shell 1.6       ==\n");
      printf("==      Copyright MalwarePad 2024      ==\n");
      printf("=========================================\n\n");
    } else if (strEql(ch, "fetch")) {
      fetch();
    } else {
      if (check_string(ch) && !strEql(ch, "exit")) {
        printf("\n%s isn't a valid command\n", ch);
      } else {
        printf("\n");
      }
    }
  } while (!strEql(ch, "exit"));

  free(ch);
}
