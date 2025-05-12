#include <bootloader.h>
#include <fb.h>
#include <malloc.h>
#include <system.h>
#include <util.h>
#include <vga.h>

// The BIOS/UEFI GOP provided framebuffer... pretty darn basic
// Copyright (C) 2024 Panagiotis

static volatile struct limine_framebuffer_request limineFBreq = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

void initiateVGA() {
  struct limine_framebuffer *framebufferRes =
      limineFBreq.response->framebuffers[0];
  fb.virt = (uint8_t *)framebufferRes->address;
  fb.phys = (size_t)fb.virt - bootloader.hhdmOffset;
  fb.height = framebufferRes->height;
  fb.width = framebufferRes->width;
  fb.pitch = framebufferRes->pitch;
  fb.bpp = framebufferRes->bpp;

  fb.red_shift = framebufferRes->red_mask_shift;
  fb.red_size = framebufferRes->red_mask_size;

  fb.green_shift = framebufferRes->green_mask_shift;
  fb.green_size = framebufferRes->green_mask_size;

  fb.blue_shift = framebufferRes->blue_mask_shift;
  fb.blue_size = framebufferRes->blue_mask_size;

  //   memcpy(&framebufferLimine, framebufferRes, sizeof(struct
  //   limine_framebuffer));
  debugf("[graphics] Resolution fixed: fb{%lx} dim(xy){%dx%d} bpp{%d}\n",
         fb.virt, fb.width, fb.height, fb.bpp);
}
