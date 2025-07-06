#include <fb.h>
#include <malloc.h>
#include <paging.h>
#include <stdarg.h>
#include <system.h>
#include <util.h>

// System framebuffer manager
// Copyright (C) 2024 Panagiotis

Framebuffer fb = {0};

void drawRect(int x, int y, int w, int h, int r, int g, int b) {
  // Optimize: minimize repeated calculations, use pointer arithmetic
  unsigned int offset = (x + y * fb.width) * 4;
  uint8_t *row = fb.virt + offset;
  for (int i = 0; i < h; i++) {
    uint8_t *pixel = row;
    for (int j = 0; j < w; j++, pixel += 4) {
      pixel[0] = b;
      pixel[1] = g;
      pixel[2] = r;
      pixel[3] = 0;
    }
    row += fb.pitch;
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
    struct fb_fix_screeninfo *fbtarg = arg;
    memcpy(fbtarg->id, "BIOS", 5);
    fbtarg->smem_start = fb.phys;
    fbtarg->smem_len = fb.width * fb.height * 4;
    fbtarg->type = FB_TYPE_PACKED_PIXELS;
    fbtarg->type_aux = 0;
    fbtarg->visual = FB_VISUAL_TRUECOLOR;
    fbtarg->xpanstep = 0;
    fbtarg->ypanstep = 0;
    fbtarg->ywrapstep = 0;
    fbtarg->line_length = fb.width * 4;
    fbtarg->mmio_start = fb.phys;
    fbtarg->mmio_len = fb.width * fb.height * 4;
    fbtarg->capabilities = 0;
    return 0;
    break;
  }
  case FBIOPUT_VSCREENINFO: {
    struct fb_var_screeninfo *fb = arg;
    (void)fb;
    // debugf("%dx%d\n", fb->xres, fb->yres);
    return 0;
    break;
  }
  case 0x4605: // FBIOPUTCMAP, ignore so no xorg.log spam
    return 0;
    break;
  case FBIOGET_VSCREENINFO: {
    struct fb_var_screeninfo *fbtarg = arg;
    fbtarg->xres = fb.width;
    fbtarg->yres = fb.height;

    fbtarg->xres_virtual = fb.width;
    fbtarg->yres_virtual = fb.height;

    fbtarg->red = (struct fb_bitfield){
        .offset = fb.red_shift, .length = fb.red_size, .msb_right = 1};
    fbtarg->green = (struct fb_bitfield){
        .offset = fb.green_shift, .length = fb.green_size, .msb_right = 1};
    fbtarg->blue = (struct fb_bitfield){
        .offset = fb.blue_shift, .length = fb.blue_size, .msb_right = 1};
    fbtarg->transp =
        (struct fb_bitfield){.offset = 24, .length = 8, .msb_right = 1};

    fbtarg->bits_per_pixel = fb.bpp;
    fbtarg->grayscale = 0;
    // fbtarg->red = 0;
    // fbtarg->green = 0;
    // fbtarg->blue = 0;
    fbtarg->nonstd = 0;
    fbtarg->activate = 0;           // idek
    fbtarg->height = fb.height / 4; // VERY approximate
    fbtarg->width = fb.width / 4;   // VERY approximate
    return 0;
    break;
  }
  default:
    return ERR(ENOTTY);
    break;
  }
}

size_t fbUserMmap(size_t addr, size_t length, int prot, int flags, OpenFile *fd,
                  size_t pgoffset) {
  if (!length)
    length = fb.width * fb.height * 4;
  size_t targPages = DivRoundUp(length, PAGE_SIZE);
  size_t physStart = fb.phys;
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
