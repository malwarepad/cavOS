#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "linux.h"

// hardcoded, xd
uint32_t width = 800;
uint32_t height = 600;

char *readFullFile(char *filename) {
  char *buffer = 0;
  long  length;
  FILE *f = fopen(filename, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }

  return buffer; // could be zero
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Wrong parameters! Usage:\n\t./drawimg ./img.cavpng\n");
    exit(1);
  }

  uint8_t                 *fbRegion = 0;
  struct fb_var_screeninfo fbInfo = {0};

  int fd = open("/dev/fb0", O_RDWR);
  if (fd < 0) {
    printf("Couldn't open framebuffer! path{/dev/fb0}\n");
    exit(1);
  }

  if (ioctl(fd, FBIOGET_VSCREENINFO, &fbInfo) < 0) {
    printf("Couldn't get framebuffer info! path{/dev/fb0} fd{%d}\n", fd);
    exit(1);
  }

  size_t fbTotalLength = fbInfo.xres * fbInfo.yres * 4;
  fbRegion =
      mmap(NULL, fbTotalLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if ((size_t)fbRegion == (size_t)(-1)) {
    printf("Couldn't mmap framebuffer! path{/dev/fb0} fd{%d}\n", fd);
    exit(1);
  }

  char *buff = readFullFile(argv[1]);
  if (!buff) {
    printf("Couldn't read file: %s\n", argv[1]);
    exit(1);
  }

  printf("\033[H\033[J"); // clear screen

  int amnt = fbInfo.yres / 16;
  for (int i = 0; i < amnt; i++)
    printf("\n");

  uint32_t max = width * height;
  for (uint32_t i = 0; i < max; ++i) {
    int bufferPos = i * 3;

    int r = buff[bufferPos];
    int g = buff[bufferPos + 1];
    int b = buff[bufferPos + 2];

    int x = i % width;
    int y = i / width;

    uint32_t offset = (x + y * fbInfo.xres) * 4;

    // bgr boii
    fbRegion[offset] = b;
    fbRegion[offset + 1] = g;
    fbRegion[offset + 2] = r;
    fbRegion[offset + 3] = 0;
  }

  free(buff);
  return 0;
}
