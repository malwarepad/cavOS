#include "../../../include/shell.h"
#define _STDINT_H
#include "../../../include/ssfn.h"

// Shell driver
// Copyright (C) 2023 Panagiotis

void launch_shell(int n) {
  string ch = (string)malloc(200);
  string data[64];
  string prompt = "$ ";

  do {
    printf("%s", prompt);
    readStr(ch); // memory_copy(readStr(), ch,100);
    if (strEql(ch, "cmd")) {
      printf("\nYou are already in cmd. A new recursive shell is opened\n");
      launch_shell(n + 1);
    } else if (strEql(ch, "clear")) {
      clearScreen();
    } else if (strEql(ch, "echo")) {
      echo(ch);
    } else if (strEql(ch, "dump")) {
      printf("\n");
      BitmapDumpBlocks();
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

      drawRect((framebufferWidth / 2) - (width / 2), ssfn_dst.y + 16, width,
               height, 255, 255, 255);
      ssfn_dst.y += height + 16;
      printf("\n");
    } else if (strEql(ch, "readfatcluster")) {
      fatCluster();
    } else if (strEql(ch, "readfatroot")) {
      fsList();
    } else if (strEql(ch, "readfatfile")) {
      fileReaderTest();
    } else if (strEql(ch, "lspci")) {
      printf("\n");
      for (uint8_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
          for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
            if (!FilterDevice(bus, slot, function))
              continue;

            PCIdevice *device = (PCIdevice *)malloc(sizeof(PCIdevice));
            GetDeviceDetails(device, 1, bus, slot, function);
            printf("[%d:%d:%d] vendor=%x device=%x class_id=%x subclass_id:%x "
                   "system_vendor_id=%x system_id=%x\n",
                   bus, slot, function, device->vendor_id, device->device_id,
                   device->class_id, device->subclass_id,
                   device->system_vendor_id, device->system_id);
            free(device);
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
    } else if (strEql(ch, "color")) {
      set_background_color();
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
      printf("==      Copyright MalwarePad 2023      ==\n");
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

void echo(string ch) {
  printf("\nInsert argument 1: ");
  char str[200];
  readStr(str);
  printf("\n%s\n", str);
}

void set_background_color() {
  // printf("\nColor codes : ");
  // printf("\n0 : black");
  // printf_colored("\n1 : blue", 1, 0); // tty.h
  // printf_colored("\n2 : green", 2, 0);
  // printf_colored("\n3 : cyan", 3, 0);
  // printf_colored("\n4 : red", 4, 0);
  // printf_colored("\n5 : purple", 5, 0);
  // printf_colored("\n6 : orange", 6, 0);
  // printf_colored("\n7 : grey", 7, 0);
  // printf_colored("\n8 : dark grey", 8, 0);
  // printf_colored("\n9 : blue light", 9, 0);
  // printf_colored("\n10 : green light", 10, 0);
  // printf_colored("\n11 : blue lighter", 11, 0);
  // printf_colored("\n12 : red light", 12, 0);
  // printf_colored("\n13 : rose", 13, 0);
  // printf_colored("\n14 : yellow", 14, 0);
  // printf_colored("\n15 : white", 15, 0);

  // printf("\n\n Text color ? : ");
  // int text_color = str_to_int(readStr());
  // printf("\n\n Background color ? : ");
  // int bg_color = str_to_int(readStr());
  // set_screen_color(text_color, bg_color);
  // clearScreen();
}

void fetch() {
  printf("\nname: cavOS");
  printf("\nmemory: %dMB", DivRoundUp(mbi_memorySizeKb, 1024));
  printf("\nuptime: %ds", DivRoundUp((uint32_t)timerTicks, 1000));
  printf("\n");
}

void help() {
  printf("\n========================== GENERIC ==========================");
  printf("\n= cmd            : Launch a new recursive Shell             =");
  printf("\n= clear          : Clears the screen                        =");
  printf("\n= echo           : Reprintf a given text                    =");
  printf("\n= exit           : Quits the current shell                  =");
  printf("\n= color          : Changes the colors of the terminal       =");
  printf("\n= fetch          : Brings you some system information       =");
  printf("\n= time           : Tells you the time and date from BIOS    =");
  printf("\n= lspci          : Lists PCI device info                    =");
  printf("\n= dump           : Dumps some of the bitmap allocator       =");
  printf("\n=============================================================\n");
  printf("\n========================= FILESYSTEM ========================");
  printf("\n= readdisk       : Tests out the disk reading algorythm     =");
  printf("\n= readfatcluster : Tests out FAT32 cluster reading          =");
  printf("\n= readfatroot    : Browse root directory  (not ready)       =");
  printf("\n= readfatfile    : Read a file's contents (not ready)       =");
  // printf("\n= readfatfile    : Browse and read files interactively      =");
  printf("\n=============================================================\n");
}

int isdigit(char c) { return c >= '0' && c <= '9'; }

void fatCluster() {
  if (!fat->works) {
    printf("\nFAT32 was not initalized properly on boot!\n");
    return;
  }

  clearScreen();
  printf("=========================================\n");
  printf("====     cavOS cluster reader 1.0    ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\nDo NOT ask for cluster 0 or 1. They simply do NOT exist on "
         "FAT32!\nCluster 2 -> starting point (/)");
  printf("\nInsert cluster number: ");

  char *choice = (char *)malloc(200);
  readStr(choice);
  int cluster = atoi(choice);
  printf("\nReading FAT cluster %d\n\r\n", cluster);
  showCluster(cluster, NULL);
  free(choice);
}

void readDisk() {
  // if (!fat->works) {
  //   printf("\nFAT32 was not initalized properly on boot!\n");
  //   return;
  // }

  clearScreen();
  printf("=========================================\n");
  printf("====        cavOS readdisk 1.0       ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\n1MB grub sector: LBA=0 Offset=0, FAT32 sector: LBA=2048 "
         "Offset=1048576");
  printf("\nInsert LBA (LBA = Offset / Sector Size): ");

  char *choice = (char *)malloc(200);
  readStr(choice);
  int lba = atoi(choice);
  printf("\nReading disk 0 with LBA=%d\n\r\n", lba);

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  for (int i = 0; i < SECTOR_SIZE; i++) {
    printf("%x ", rawArr[i]);
  }

  printf("\r\n");
  free(rawArr);
  free(choice);
}

void fsList() {
  if (!fat->works) {
    printf("\nFAT32 was not initalized properly on boot!\n");
    return;
  }

  clearScreen();
  printf("=========================================\n");
  printf("====        cavOS readroot 1.0       ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  int cluster = 2;
  int previous = 0;
  // unsigned char *innArr;
  while (cluster >= 2) {
    showCluster(cluster, 0); // 0x10

    printf("\nEnter root directory choice (folder name OR '}' to exit):");
    printf("\n> ");
    char choice[200];
    readStr(choice);
    if (strlength(choice) > 11) {
      printf("\nInvalid options!\n");
      continue;
    }

    if (strEql(choice, "}")) {
      cluster = 0;
      printf("\n");
      break;
    } else if (strEql(choice, "{")) {
      clearScreen();
      continue;
    } else if (strEql(choice, "..") && previous != 0) {
      cluster = previous;
    }

    printf("\n");

    int lba = fat->cluster_begin_lba + (cluster - 2) * fat->sectors_per_cluster;

    int more = 1;
    while (more) {
      uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
      getDiskBytes(rawArr, lba, 1);
      for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
        if (rawArr[32 * i] == 0) {
          break;
          more = 0;
        }
        for (int o = 0; o < 11; o++) {
          // todo better alg
          if ((rawArr[32 * i + o] == choice[o] ||
               (rawArr[32 * i + o] == 0x20) && o == 10)) {
            // cluster = ((uint32_t)rawArr[32 * i + 20] << 24) |
            //           ((uint32_t)rawArr[32 * i + 26] << 16) |
            //           ((uint32_t)rawArr[32 * i + 21] << 8) | rawArr[32 * i +
            //           27];
            previous = cluster;
            cluster = rawArr[32 * i + 26] | (rawArr[32 * i + 27] << 8);
            printf("\n");
            // printf("Hexadecimal: %x, Decimal:%d, {%x %x %x %x}\n", cluster,
            //        cluster, rawArr[32 * i + 20], rawArr[32 * i + 21],
            //        rawArr[32 * i + 26], rawArr[32 * i + 27]);
            o = 12;
            i = (SECTOR_SIZE / 32) + 1;
            more = 0;
          } else if (rawArr[32 * i + o] != choice[o] &&
                     rawArr[32 * i + o] != 0x20) {
            o = 12;
          }
        }
      }
      if (more && rawArr[SECTOR_SIZE - 32] != 0) {
        unsigned int newCluster = getFatEntry(cluster);
        if (newCluster == 0) {
          more = 0;
          break;
        }
        cluster = newCluster;
        lba = fat->cluster_begin_lba + (cluster - 2) * fat->sectors_per_cluster;
        more = 1;
      } else
        more = 0;
      free(rawArr);
    }
  }
}

// void fsRead() { // todo make this work

//   clearScreen();
//   printf("=========================================\n");
//   printf("====        cavOS readroot 1.0       ====\n");
//   printf("====    Copyright MalwarePad 2023    ====\n");
//   printf("=========================================\n");

//   printf("\nRead file by cluster: ");
//   string cluster = atoi(readStr());
// }
