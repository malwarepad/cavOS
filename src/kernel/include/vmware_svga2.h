#include "pci.h"
#include "types.h"

#ifndef VMWARE_SVGA_II_H
#define VMWARE_SVGA_II_H

void initiateVMWareSvga2(PCIdevice *device);

#define SVGA_INDEX 0
#define SVGA_VALUE 1
#define SVGA_BIOS 2
#define SVGA_IRQSTATUS 8

typedef struct VMWareSvga {
  bool   exists;
  size_t base;

  uint32_t *fifo;
} VMWareSvga;

#define SVGA_REG_ID 0         // register used to negociate specification ID
#define SVGA_REG_ENABLE 1     // flag set when the device should enter SVGA mode
#define SVGA_REG_WIDTH 2      // current screen width
#define SVGA_REG_HEIGHT 3     // current screen height
#define SVGA_REG_MAX_WIDTH 4  // maximum supported screen width
#define SVGA_REG_MAX_HEIGHT 5 // maximum supported screen height
#define SVGA_REG_DEPTH 6
#define SVGA_REG_BPP 7 // current screen bits per pixel
#define SVGA_REG_PSEUDOCOLOR 8
#define SVGA_REG_RED_MASK 9
#define SVGA_REG_GREEN_MASK 10
#define SVGA_REG_BLUE_MASK 11
#define SVGA_REG_BYTES_PER_LINE 12
#define SVGA_REG_FB_START 13  // address in system memory of the frame buffer
#define SVGA_REG_FB_OFFSET 14 // offset in framebuffer to the visible pixel data
#define SVGA_REG_VRAM_SIZE 15 // size of the video RAM
#define SVGA_REG_FB_SIZE 16   // size of the frame buffer
#define SVGA_REG_CAPABILITIES 17 // device capabilities
#define SVGA_REG_FIFO_START 18   // address in system memory of the FIFO
#define SVGA_REG_FIFO_SIZE 19    // FIFO size
#define SVGA_REG_CONFIG_DONE 20  // flag to enable FIFO operation
#define SVGA_REG_SYNC 21         // flag set by the driver to flush FIFO changes
#define SVGA_REG_BUSY 22         // flag set by the FIFO when it's processed

#define SVGA_FIFO_MIN 0      // start of command queue
#define SVGA_FIFO_MAX 1      // end of command queue
#define SVGA_FIFO_NEXT_CMD 2 // next command (offset in bytes)
#define SVGA_FIFO_STOP 3     // todo: explain what SVGA_FIFO_STOP does

#define SVGA_GUEST_OS_BASE 0x5000

#define SVGA_GUEST_OS_DOS (SVGA_GUEST_OS_BASE + 1)
#define SVGA_GUEST_OS_WIN31 (SVGA_GUEST_OS_BASE + 2)
#define SVGA_GUEST_OS_WINDOWS95 (SVGA_GUEST_OS_BASE + 3)
#define SVGA_GUEST_OS_WINDOWS98 (SVGA_GUEST_OS_BASE + 4)
#define SVGA_GUEST_OS_WINDOWSME (SVGA_GUEST_OS_BASE + 5)
#define SVGA_GUEST_OS_NT (SVGA_GUEST_OS_BASE + 6)
#define SVGA_GUEST_OS_WIN2000 (SVGA_GUEST_OS_BASE + 7)
#define SVGA_GUEST_OS_LINUX (SVGA_GUEST_OS_BASE + 8)
#define SVGA_GUEST_OS_OS2 (SVGA_GUEST_OS_BASE + 9)
#define SVGA_GUEST_OS_OTHER (SVGA_GUEST_OS_BASE + 10)
#define SVGA_GUEST_OS_FREEBSD (SVGA_GUEST_OS_BASE + 11)
#define SVGA_GUEST_OS_WHISTLER (SVGA_GUEST_OS_BASE + 12)

#endif
