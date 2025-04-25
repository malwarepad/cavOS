#include <bootloader.h>
#include <console.h>
#include <fb.h>
#include <malloc.h>
#include <paging.h>
#include <string.h>
#include <system.h>
#include <timer.h>
#include <util.h>
#include <vga.h>
#include <vmware_svga2.h>

// VMWare SVGA-II driver (despite the name it used also used in qemu)
// Copyright (C) 2025 Panagiotis

// todo: cleaner global framebuffer struct so this can be used properly

VMWareSvga VMWareSvga2 = {0};

bool VMWareSvga2Detect(PCIdevice *device) {
  return device->vendor_id == 0x15ad && device->device_id == 0x0405;
}

uint32_t VMWareSvga2Read(uint32_t index) {
  outportl(VMWareSvga2.base + SVGA_INDEX, index);
  return inportl(VMWareSvga2.base + SVGA_VALUE);
}

void VMWareSvga2Write(uint32_t index, uint32_t value) {
  outportl(VMWareSvga2.base + SVGA_INDEX, index);
  outportl(VMWareSvga2.base + SVGA_VALUE, value);
}

void VMwareSvga2SetMode(uint32_t width, uint32_t height, uint32_t bpp) {
  assert(width <= VMWareSvga2Read(SVGA_REG_MAX_WIDTH) &&
         height <= VMWareSvga2Read(SVGA_REG_MAX_HEIGHT));
  framebufferHeight = height;
  framebufferWidth = width;
  framebufferPitch = framebufferWidth * 4;
  VMWareSvga2Write(SVGA_REG_WIDTH, width);
  VMWareSvga2Write(SVGA_REG_HEIGHT, height);
  VMWareSvga2Write(SVGA_REG_BPP, bpp);

  assert(VMWareSvga2Read(SVGA_REG_WIDTH) == width);
  assert(VMWareSvga2Read(SVGA_REG_HEIGHT) == height);
  VMWareSvga2Read(SVGA_REG_BPP);

  // todo: store everything in the upcoming struct (on sync!!)
  VMWareSvga2Read(SVGA_REG_BYTES_PER_LINE);
  VMWareSvga2Read(SVGA_REG_DEPTH);
  VMWareSvga2Read(SVGA_REG_PSEUDOCOLOR);
  VMWareSvga2Read(SVGA_REG_RED_MASK);
  VMWareSvga2Read(SVGA_REG_GREEN_MASK);
  VMWareSvga2Read(SVGA_REG_BLUE_MASK);
}

void VMwareSvga2Sync() {
  size_t phys =
      VMWareSvga2Read(SVGA_REG_FB_START) + VMWareSvga2Read(SVGA_REG_FB_OFFSET);
  size_t addr = 0x500000000000; // <- todo
  size_t pages = DivRoundUp(VMWareSvga2Read(SVGA_REG_VRAM_SIZE), PAGE_SIZE);
  for (int i = 0; i < pages; i++) {
    VirtualMap(addr + i * PAGE_SIZE, phys + i * PAGE_SIZE, PF_RW | PF_CACHE_WC);
  }
  framebuffer = (uint8_t *)addr;

  uint32_t bpp = VMWareSvga2Read(SVGA_REG_BPP);
  assert(bpp == 32);
  framebufferHeight = VMWareSvga2Read(SVGA_REG_HEIGHT);
  framebufferWidth = VMWareSvga2Read(SVGA_REG_WIDTH);
  framebufferPitch = framebufferWidth * 4;

  debugf("[pci::svga-II] Syncing: fb{%lx:%lx} dim(xy){%dx%d} bpp{%d}\n", phys,
         framebuffer, framebufferWidth, framebufferHeight, bpp);
}

void VMwareSvga2FifoWrite(uint32_t value) {
  // todo: ensure atomic operation (if it's on the timer too for instance)
  uint32_t next_cmd = VMWareSvga2.fifo[SVGA_FIFO_NEXT_CMD];
  assert((next_cmd % 4) == 0);
  VMWareSvga2.fifo[next_cmd / 4] = value;
  next_cmd += 4;

  if (next_cmd >= VMWareSvga2.fifo[SVGA_FIFO_MAX])
    next_cmd = VMWareSvga2.fifo[SVGA_FIFO_MIN];

  VMWareSvga2.fifo[SVGA_FIFO_NEXT_CMD] = next_cmd;
}

#define SVGA_CMD_UPDATE 1
void VMwareSvga2Update(uint32_t x, uint32_t y, uint32_t width,
                       uint32_t height) {
  VMwareSvga2FifoWrite(SVGA_CMD_UPDATE);
  VMwareSvga2FifoWrite(x);
  VMwareSvga2FifoWrite(y);
  VMwareSvga2FifoWrite(width);
  VMwareSvga2FifoWrite(height);
}

void initiateVMWareSvga2(PCIdevice *device) {
  if (!VMWareSvga2Detect(device))
    return;

  debugf("[pci::svga-II] VMWare SVGA-II graphics card detected!\n");
  return; // todo

  assert(!VMWareSvga2.exists);
  VMWareSvga2.exists = true;

  PCIgeneralDevice *details =
      (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));
  GetGeneralDevice(device, details);
  VMWareSvga2.base = details->bar[0] & (~0x3);
  free(details); // not needed anymore

  // enable needed stuff on the PCI spec
  uint32_t command_status = COMBINE_WORD(device->status, device->command);
  command_status |= 0x7;
  ConfigWriteDword(device->bus, device->slot, device->function, PCI_COMMAND,
                   command_status);

  // latest specification ID
  VMWareSvga2Write(SVGA_REG_ID, 0x90000002);
  assert(VMWareSvga2Read(SVGA_REG_ID) == 0x90000002);
  // VMWareSvga2Write(SVGA_REG_SYNC, 1); // extra

  // fake being linux (supposedly unlocks extra functionality)
  VMWareSvga2Write(23, SVGA_GUEST_OS_LINUX);

  // "map" the fifo correctly
  VMWareSvga2.fifo = (uint32_t *)(bootloader.hhdmOffset +
                                  VMWareSvga2Read(SVGA_REG_FIFO_START));

  // use the fifo
  VMWareSvga2.fifo[SVGA_FIFO_MIN] = 293 * 4;
  VMWareSvga2.fifo[SVGA_FIFO_MAX] = VMWareSvga2Read(SVGA_REG_FIFO_SIZE);
  VMWareSvga2.fifo[SVGA_FIFO_NEXT_CMD] = 293 * 4;
  VMWareSvga2.fifo[SVGA_FIFO_STOP] = 293 * 4;

  debugf("%ld %ld\n", VMWareSvga2Read(SVGA_REG_MAX_WIDTH),
         VMWareSvga2Read(SVGA_REG_MAX_HEIGHT));

  debugf("vram: %ld bytes\n", VMWareSvga2Read(SVGA_REG_VRAM_SIZE));

  // VMwareSvga2SetMode(VMWareSvga2Read(SVGA_REG_WIDTH),
  //                    VMWareSvga2Read(SVGA_REG_HEIGHT), 32);
  VMwareSvga2SetMode(1920, 1080, 32);

  VMWareSvga2Write(SVGA_REG_CONFIG_DONE, 1);
  VMWareSvga2Write(SVGA_REG_ENABLE, 1);
  assert(VMWareSvga2Read(SVGA_REG_ENABLE) == 1);
  assert(VMWareSvga2Read(SVGA_REG_CONFIG_DONE) == 1);

  VMwareSvga2Sync();

  drawRect(0, 0, framebufferWidth, framebufferHeight, 255, 255, 0);
  VMwareSvga2Update(0, 0, framebufferWidth, framebufferHeight);
}
