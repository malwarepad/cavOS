#include "../../../include/shell.h"

// Shell driver
// Copyright (C) 2023 Panagiotis

#define REGULAR_ENV 0
#define FS_ENV 1
int activeEnv = REGULAR_ENV; // 0 -> regular, 1 -> filesystem

int gateKeepEnvs(int target) {
  if (activeEnv != target) {
    printf("\nWrong env! This command requires %d, while you're inside %d!\n",
           target, activeEnv);
    return 0;
  }

  return 1;
}

void launch_shell(int n, multiboot_info_t *mbi) {
  string ch = (string)malloc(200);
  string data[64];
  string prompt = "$ ";

  do {
    if (activeEnv == FS_ENV)
      printf("(%d) %s", activeEnv, prompt);
    else
      printf("%s", prompt);
    ch = readStr(); // memory_copy(readStr(), ch,100);
    if (strEql(ch, "cmd")) {
      printf("\nYou are already in cmd. A new recursive shell is opened\n");
      launch_shell(n + 1, mbi);
    } else if (strEql(ch, "clear")) {
      clearScreen();
    } else if (strEql(ch, "echo")) {
      echo(ch);
    } else if (strEql(ch, "help")) {
      help();
    } else if (strEql(ch, "readdisk")) {
      readDisk();
    } else if (strEql(ch, "env")) {
      envChange();
    } else if (strEql(ch, "readfatcluster")) {
      fatCluster();
    } else if (strEql(ch, "readfatroot")) {
      fsList();
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
      fetch(mbi);
    } else {
      if (check_string(ch) && !strEql(ch, "exit")) {
        printf("\n%s isn't a valid command\n", ch);
      } else {
        printf("\n");
      }
    }
  } while (!strEql(ch, "exit"));
}

void echo(string ch) {
  printf("\nInsert argument 1: ");
  printf("\n%s\n", readStr());
}

void set_background_color() {
  printf("\nColor codes : ");
  printf("\n0 : black");
  printf_colored("\n1 : blue", 1, 0); // tty.h
  printf_colored("\n2 : green", 2, 0);
  printf_colored("\n3 : cyan", 3, 0);
  printf_colored("\n4 : red", 4, 0);
  printf_colored("\n5 : purple", 5, 0);
  printf_colored("\n6 : orange", 6, 0);
  printf_colored("\n7 : grey", 7, 0);
  printf_colored("\n8 : dark grey", 8, 0);
  printf_colored("\n9 : blue light", 9, 0);
  printf_colored("\n10 : green light", 10, 0);
  printf_colored("\n11 : blue lighter", 11, 0);
  printf_colored("\n12 : red light", 12, 0);
  printf_colored("\n13 : rose", 13, 0);
  printf_colored("\n14 : yellow", 14, 0);
  printf_colored("\n15 : white", 15, 0);

  printf("\n\n Text color ? : ");
  int text_color = str_to_int(readStr());
  printf("\n\n Background color ? : ");
  int bg_color = str_to_int(readStr());
  set_screen_color(text_color, bg_color);
  clearScreen();
}

void fetch(multiboot_info_t *mbi) {
  printf("\nname: cavOS");
  printf("\nmemory: %dMB", ((mbi->mem_upper) / 1024));
  printf("\n");
}

void help() {
  printf("\n========================== GENERIC ==========================");
  printf("\n= env            : Changes your environment                 =");
  printf("\n= cmd            : Launch a new recursive Shell             =");
  printf("\n= clear          : Clears the screen                        =");
  printf("\n= echo           : Reprintf a given text                    =");
  printf("\n= exit           : Quits the current shell                  =");
  printf("\n= color          : Changes the colors of the terminal       =");
  printf("\n= fetch          : Brings you some system information       =");
  printf("\n=============================================================\n");
  printf("\n========================= FILESYSTEM ========================");
  printf("\n= readdisk       : (1) Tests out the disk reading algorythm =");
  printf("\n= readfatcluster : (1) Tests out FAT32 cluster reading      =");
  printf("\n= readfatroot    : (1) Browse root directory (not ready)    =");
  // printf("\n= readfatfile    : (1) Browse and read files interactively  =");
  printf("\n=============================================================\n");
}

int isdigit(char c) { return c >= '0' && c <= '9'; }

int atoi(const char *str) {
  int value = 0;
  while (isdigit(*str)) {
    value *= 10;
    value += (*str) - '0';
    str++;
  }

  return value;
}

void envChange() {
  printf("\nChange your environment (0 -> regular, 1 -> filesystem): ");
  int newEnv = atoi(readStr());

  if (newEnv == FS_ENV && !fat.works) {
    printf("\nFAT32 was not initalized properly on boot!\n");
    return;
  }

  clearScreen();
  activeEnv = newEnv;
  printf("Changed into %d environment.\n\n", newEnv);
}

void fatCluster() {
  if (!gateKeepEnvs(FS_ENV))
    return;

  clearScreen();
  printf("=========================================\n");
  printf("====     cavOS cluster reader 1.0    ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\nDo NOT ask for cluster 0 or 1. They simply do NOT exist on "
         "FAT32!\nCluster 2 -> starting point (/)");
  printf("\nInsert cluster number: ");

  int cluster = atoi(readStr());
  printf("\nReading FAT cluster %d\n\r\n", cluster);
  showCluster(cluster, NULL);
}

void readDisk() {
  if (!gateKeepEnvs(FS_ENV))
    return;

  clearScreen();
  printf("=========================================\n");
  printf("====        cavOS readdisk 1.0       ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\n1MB grub sector: LBA=0 Offset=0, FAT32 sector: LBA=2048 "
         "Offset=1048576");
  printf("\nInsert LBA (LBA = Offset / Sector Size): ");

  int lba = atoi(readStr());
  printf("\nReading disk 0 with LBA=%d\n\r\n", lba);

  unsigned char *rawArr;
  getDiskBytes(rawArr, lba, 1);

  for (int i = 0; i < SECTOR_SIZE; i++) {
    printf("%x ", rawArr[i]);
  }

  printf("\r\n");
}

void fsList() {
  if (!gateKeepEnvs(FS_ENV))
    return;

  clearScreen();
  printf("=========================================\n");
  printf("====        cavOS readroot 1.0       ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  int            cluster = 2;
  int            reading = 1;
  unsigned char *rawArr;
  // unsigned char *innArr;
  while (reading) {
    showCluster(cluster, 0); // 0x10

    printf("\nEnter root directory choice (folder name OR '}' to exit):");
    printf("\n> ");
    string choice = readStr();
    if (strlength(choice) > 11) {
      printf("\nInvalid options!\n");
      continue;
    }

    if (strEql(choice, "}")) {
      reading = 0;
      printf("\n");
      break;
    }

    printf("\n");

    int lba = fat.cluster_begin_lba + (cluster - 2) * fat.sectors_per_cluster;

    int more = 1;
    while (more) {
      getDiskBytes(rawArr, lba, 1);
      for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
        if (rawArr[32 * i] == 0) {
          break;
          more = 0;
        }
        for (int o = 0; o < 11; o++) {
          // todo better alg
          if ((rawArr[32 * i + o] == choice[o] || rawArr[32 * i + o] == 0x20) &&
              o == 10) {
            // cluster = ((uint32_t)rawArr[32 * i + 20] << 24) |
            //           ((uint32_t)rawArr[32 * i + 26] << 16) |
            //           ((uint32_t)rawArr[32 * i + 21] << 8) | rawArr[32 * i +
            //           27];
            cluster = rawArr[32 * i + 26] | (rawArr[32 * i + 27] << 8);
            printf("Hexadecimal: %x, Decimal:%d, {%x %x %x %x}\n", cluster,
                   cluster, rawArr[32 * i + 20], rawArr[32 * i + 21],
                   rawArr[32 * i + 26], rawArr[32 * i + 27]);
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
        lba = fat.cluster_begin_lba + (cluster - 2) * fat.sectors_per_cluster;
        more = 1;
      } else
        more = 0;
    }
  }
}

// void fsRead() { // todo make this work
//   if (!gateKeepEnvs(FS_ENV))
//     return;

//   clearScreen();
//   printf("=========================================\n");
//   printf("====        cavOS readroot 1.0       ====\n");
//   printf("====    Copyright MalwarePad 2023    ====\n");
//   printf("=========================================\n");

//   printf("\nRead file by cluster: ");
//   string cluster = atoi(readStr());
// }
