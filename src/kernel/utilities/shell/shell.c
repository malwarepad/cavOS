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

void launch_shell(int n) { assert(false); }
