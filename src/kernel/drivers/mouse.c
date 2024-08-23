#include <fb.h>
#include <isr.h>
#include <mouse.h>
#include <system.h>
#include <util.h>

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

void mouseIrq() {
  uint8_t byte = mouseRead();

  // debugf("%d %d %d\n", byte1, byte2, byte3);
  if (mouseCycle == 0)
    mouse1 = byte;
  else if (mouseCycle == 1)
    mouse2 = byte;
  else {
    int mouse3 = byte;

    do {
      // if (byte & (1 << 6) || byte & (1 << 7))
      //   break;

      // debugf("%d ", !!(mouse1 & (1 << 0)));

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
  mouseWait(0);
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
  registerIRQhandler(12, mouseIrq);
}
