#include <apic.h>
#include <fb.h>
#include <isr.h>
#include <mouse.h>
#include <system.h>
#include <timer.h>
#include <util.h>
#include <vga.h>

// PS/2 mouse driver, should work with USB mice too
// Copyright (C) 2024 Panagiotis

void mouseWait(uint8_t a_type) {
  uint32_t timeout = MOUSE_TIMEOUT;
  if (!a_type) {
    while (--timeout) {
      if (inportb(MOUSE_STATUS) & MOUSE_BBIT)
        break;
    }
  } else {
    while (--timeout) {
      if (!((inportb(MOUSE_STATUS) & MOUSE_ABIT)))
        break;
    }
  }
}

void mouseWrite(uint8_t write) {
  mouseWait(1);
  outportb(MOUSE_STATUS, MOUSE_WRITE);
  mouseWait(1);
  outportb(MOUSE_PORT, write);
}

uint8_t mouseRead() {
  mouseWait(0);
  char t = inportb(MOUSE_PORT);
  return t;
}

int mouseCycle = 0;

int mouse1 = 0;
int mouse2 = 0;

int gx = 0;
int gy = 0;

void mouseIrq() {
  uint8_t byte = mouseRead();
  return;
  // rest are just for demonstration

  // debugf("%d %d %d\n", byte1, byte2, byte3);
  if (mouseCycle == 0)
    mouse1 = byte;
  else if (mouseCycle == 1)
    mouse2 = byte;
  else {
    int mouse3 = byte;

    do {
      assert(mouse1 & (1 << 3));
      // if (byte & (1 << 6) || byte & (1 << 7))
      //   break;

      int x = mouse2;
      int y = mouse3;
      if (x && mouse1 & (1 << 4))
        x -= 0x100;
      if (y && mouse1 & (1 << 5))
        y -= 0x100;

      gx += x;
      gy += -y;
      if (gx < 0)
        gx = 0;
      if (gy < 0)
        gy = 0;
      if (gx > framebufferWidth)
        gx = framebufferWidth;
      if (gy > framebufferHeight)
        gy = framebufferHeight;

      bool click = mouse1 & (1 << 0);
      bool rclick = mouse1 & (1 << 1);
      drawRect(0, 0, framebufferWidth, framebufferHeight, 255, 255, 255);
      drawRect(gx, gy, 20, 20, click ? 255 : 0, rclick ? 255 : 0, 0);

      // debugf("(%d, %d)", gx, gy);

      // do nothing... seriously!
      (void)mouse3;
    } while (0);
  }

  mouseCycle++;
  if (mouseCycle > 2)
    mouseCycle = 0;
}

void initiateMouse() {
  // enable the auxiliary mouse
  mouseWait(1);
  outportb(0x64, 0xA8);

  // enable interrupts
  mouseWait(1);
  outportb(0x64, 0x20);
  sleep(100);
  mouseWait(0);
  sleep(100);
  uint8_t status;
  status = (inportb(0x60) | 2);
  mouseWait(1);
  outportb(0x64, 0x60);
  mouseWait(1);
  outportb(0x60, status);

  // default settings
  mouseWrite(0xF6);
  mouseRead();

  // enable device
  mouseWrite(0xF4);
  mouseRead();

  // irq handler
  uint8_t targIrq = ioApicRedirect(12, false);
  registerIRQhandler(targIrq, mouseIrq);
}
