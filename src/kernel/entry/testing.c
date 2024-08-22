#include <arp.h>
#include <elf.h>
#include <fb.h>
#include <icmp.h>
#include <malloc.h>
#include <md5.h>
#include <ne2k.h>
#include <pci.h>
#include <socket.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <tcp.h>
#include <testing.h>
#include <udp.h>
#include <util.h>
#include <vmm.h>

#include <timer.h>
#include <vfs.h>

// Testing stuff
// Copyright (C) 2024 Panagiotis

extern void weirdTests();

// char *argv[] = {"/doom", "-iwad", "/DOOM.WAD"};
// char *argv[] = {"/usr/bin/busybox", "sh"};
// char *argv[] = {"/usr/bin/bash"};
// char *argv[] = {"/usr/bin/testing"};
// char *argv[] = {"/a.out"};
// char *argv[] = {"/usr/bin/doom", "-iwad", "/usr/bin/doom.wad"};
void testingInit() {
  // netSocketConnect(selectedNIC, SOCKET_PROT_UDP, (uint8_t[]){10, 0, 2, 15},
  //                   5643, 69);
  // weirdTests();
  // elfExecute("/usr/bin/busybox", 2, argv);
  // while (1) {
  // Task *task =
  //     elfExecute(argv[0], sizeof(argv) / sizeof(argv[0]), argv, 0, 0, true);
  // while (taskGetState(task->id))
  //   ;
  // }
}

void weirdTests() {
  // char fn[] = "hehe/hehe2/fuck/./././////..//./////./././..//./././";
  // printf("ticks before: %ld\n", timerTicks);
  // printf("%s\n", fsSanitize(fn));
  // printf("ticks before: %ld\n", timerTicks);
  // panic();

  // fsMount("/test/", CONNECTOR_DUMMY, 0, 0);

  uint32_t targA, targB, targC, targD;
  for (int i = 0; i < 0; i++) {
    OpenFile *dir = fsKernelOpen("/files/lorem.txt", FS_MODE_READ, 0);
    if (!dir) {
      printf("File cannot be found!\n");
      continue;
    }
    uint32_t filesize = fsGetFilesize(dir);
    uint8_t *out = (uint8_t *)malloc(filesize);
    fsReadFullFile(dir, out);
    fsKernelClose(dir);

    // uint32_t filesize = sizeof(FAT32_Directory);
    // char    *out = dir->dir;

    // uint8_t *out = malloc(lengthWoo);
    // uint32_t filesize = lengthWoo;
    // memcpy(out, targ, filesize);

    // uint32_t filesize = 512;
    // uint8_t *out = malloc(filesize);
    // getDiskBytes(out, 0, 1);

    MD5_CTX *ctx = malloc(sizeof(MD5_CTX));
    MD5_Init(ctx);
    MD5_Update(ctx, out, filesize);

    MD5_OUT *md = malloc(sizeof(MD5_CTX));
    MD5_Final((void *)md, ctx);
    if (i != 0 && (md->a != targA || md->b != targB || md->c != targC ||
                   md->d != targD)) {
      debugf("FAIL! FAIL! FAIL!\n");
      break;
    }
    targA = md->a;
    targB = md->b;
    targC = md->c;
    targD = md->d;
    debugf("%02x%02x%02x%02x\n", md->a, md->b, md->c, md->d);

    // for (int i = 0; i < filesize; i++) {
    //   printf("%02X ", out[i]);
    // }
    // printf("\n");
  }
}