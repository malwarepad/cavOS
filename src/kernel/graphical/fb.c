#include <fb.h>
#include <malloc.h>
#include <paging.h>
#include <stdarg.h>
#include <system.h>
#include <util.h>

// System framebuffer manager
// Copyright (C) 2024 Panagiotis

uint8_t *framebuffer = 0;

uint16_t framebufferWidth;
uint16_t framebufferHeight;
uint32_t framebufferPitch;

void drawRect(int x, int y, int w, int h, int r, int g,
              int b) { // Draw a filled rectangle
  unsigned int offset =
      (x + y * framebufferWidth) *
      4; // Finding the location of the pixel in the video array
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) { // color each line
      framebuffer[offset + j * 4] = b;
      framebuffer[offset + j * 4 + 1] = g;
      framebuffer[offset + j * 4 + 2] = r;
      framebuffer[offset + j * 4 + 3] = 0;
    }
    offset += framebufferPitch; // switch to the beginnering of next line
  }
}

size_t fbUserIllegal() {
  debugf("[io::fb] Tried to do anything but an mmap/ioctl!\n");
  return -1;
}

size_t fbUserIoctl(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  // case FBIOGETCMAP: {
  //   struct fb_cmap *cmap = arg;
  //   cmap->start = 0;
  //   cmap->len = 8;
  //   for (int i = 0; i < 8; i++)
  //     cmap->red[i] = (((2 * i) + 1) * (0xFFFF)) / 16;
  //   memcpy(cmap->green, cmap->red, sizeof(uint16_t) * 8);
  //   memcpy(cmap->blue, cmap->red, sizeof(uint16_t) * 8);
  //   return 0;
  // }
  case FBIOGET_FSCREENINFO: {
    struct fb_fix_screeninfo *fb = arg;
    memcpy(fb->id, "BIOS", 5);
    fb->smem_start = VirtualToPhysical((size_t)framebuffer);
    fb->smem_len = framebufferWidth * framebufferHeight * 4;
    fb->type = FB_TYPE_VGA_PLANES;
    fb->type_aux = FB_TYPE_VGA_PLANES;
    fb->visual = FB_VISUAL_TRUECOLOR;
    fb->xpanstep = 0;
    fb->ypanstep = 0;
    fb->ywrapstep = 0;
    fb->line_length = framebufferWidth * 4;
    fb->mmio_start = VirtualToPhysical((size_t)framebuffer);
    fb->mmio_len = framebufferWidth * framebufferHeight * 4;
    fb->capabilities = 0;
    return 0;
  }
  case FBIOGET_VSCREENINFO: {
    struct fb_var_screeninfo *fb = arg;
    fb->xres = framebufferWidth;
    fb->yres = framebufferHeight;

    fb->xres_virtual = framebufferWidth;
    fb->yres_virtual = framebufferHeight;

    fb->bits_per_pixel = 32;
    fb->grayscale = 0;
    // fb->red = 0;
    // fb->green = 0;
    // fb->blue = 0;
    fb->nonstd = 0;
    fb->activate = 0;                   // idek
    fb->height = framebufferHeight / 4; // VERY approximate
    fb->width = framebufferWidth / 4;   // VERY approximate
    return 0;
    break;
  }
  default:
    return -1;
    break;
  }
}

size_t fbUserMmap(size_t addr, size_t length, int prot, int flags, OpenFile *fd,
                  size_t pgoffset) {
  if (!length)
    length = framebufferWidth * framebufferHeight * 4;
  size_t targPages = DivRoundUp(length, PAGE_SIZE);
  size_t physStart = VirtualToPhysical((size_t)framebuffer);
  for (int i = 0; i < targPages; i++) {
    VirtualMap(0x150000000000 + i * PAGE_SIZE, physStart + i * PAGE_SIZE,
               PF_RW | PF_USER | PF_CACHE_WC);
  } // todo: get rid of hardcoded location!
  return 0x150000000000;
}

size_t fbUserStat(OpenFile *fd, stat *target) {
  target->st_dev = 70;
  target->st_ino = rand(); // todo!
  target->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
  target->st_nlink = 1;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = 0x1000;
  target->st_size = 0;
  target->st_blocks = DivRoundUp(target->st_size, 512);
  target->st_atime = 69;
  target->st_mtime = 69;
  target->st_ctime = 69;

  return 0;
}

VfsHandlers fb0 = {.open = 0,
                   .close = 0,
                   .read = fbUserIllegal,
                   .write = fbUserIllegal,
                   .ioctl = fbUserIoctl,
                   .mmap = fbUserMmap,
                   .stat = fbUserStat,
                   .duplicate = 0,
                   .getdents64 = 0};
