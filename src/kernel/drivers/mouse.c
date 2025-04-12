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

bool clickedLeft = false;
bool clickedRight = true;

DevInputEvent *mouseEvent;

void mouseIrq() {
  uint8_t byte = mouseRead();
  // return;
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
      // drawRect(0, 0, framebufferWidth, framebufferHeight, 255, 255, 255);
      // drawRect(gx, gy, 20, 20, click ? 255 : 0, rclick ? 255 : 0, 0);

      if (clickedLeft && !click)
        inputGenerateEvent(mouseEvent, EV_KEY, BTN_LEFT, 0);
      if (!clickedLeft && click)
        inputGenerateEvent(mouseEvent, EV_KEY, BTN_LEFT, 1);

      if (clickedRight && !rclick)
        inputGenerateEvent(mouseEvent, EV_KEY, BTN_RIGHT, 0);
      if (!clickedRight && rclick)
        inputGenerateEvent(mouseEvent, EV_KEY, BTN_RIGHT, 1);

      clickedRight = rclick;
      clickedLeft = click;

      // inputGenerateEvent(mouseEvent, EV_ABS, ABS_X, gx);
      // inputGenerateEvent(mouseEvent, EV_ABS, ABS_Y, gy);
      inputGenerateEvent(mouseEvent, EV_REL, REL_X, x);
      inputGenerateEvent(mouseEvent, EV_REL, REL_Y, -y);
      inputGenerateEvent(mouseEvent, EV_SYN, SYN_REPORT, 0);

      // debugf("(%d, %d)", gx, gy);

      // do nothing... seriously!
      (void)mouse3;
    } while (0);
  }

  mouseCycle++;
  if (mouseCycle > 2)
    mouseCycle = 0;
}

void bitmapset(uint8_t *map, size_t toSet) {
  size_t div = toSet / 8;
  size_t mod = toSet % 8;
  map[div] |= (1 << mod);
}

size_t mouseEventBit(OpenFile *fd, uint64_t request, void *arg) {
  size_t number = _IOC_NR(request);
  size_t size = _IOC_SIZE(request);

  size_t ret = ERR(ENOENT);
  switch (number) {
  case 0x20: {
    size_t out = (1 << EV_SYN) | (1 << EV_KEY) | (1 << EV_REL);
    ret = MIN(sizeof(size_t), size);
    memcpy(arg, &out, ret);
    break;
  }
  case (0x20 + EV_SW):
  case (0x20 + EV_MSC):
  case (0x20 + EV_SND):
  case (0x20 + EV_LED):
  case (0x20 + EV_ABS): {
    ret = MIN(sizeof(size_t), size);
    break;
  }
  case (0x20 + EV_FF): {
    ret = MIN(16, size);
    break;
  }
  case (0x20 + EV_REL): {
    size_t out = (1 << REL_X) | (1 << REL_Y);
    ret = MIN(sizeof(size_t), size);
    memcpy(arg, &out, ret);
    break;
  }
  case (0x20 + EV_KEY): {
    uint8_t map[96] = {0};
    bitmapset(map, BTN_RIGHT);
    bitmapset(map, BTN_LEFT);
    ret = MIN(96, size);
    memcpy(arg, map, ret);
    break;
  }
  case (0x40 + ABS_X): {
    assert(size >= sizeof(struct input_absinfo));
    struct input_absinfo *target = (struct input_absinfo *)arg;
    memset(target, 0, sizeof(struct input_absinfo));
    target->value = 0; // todo
    target->minimum = 0;
    target->maximum = framebufferWidth;
    ret = 0;
    break;
  }
  case (0x40 + ABS_Y): {
    assert(size >= sizeof(struct input_absinfo));
    struct input_absinfo *target = (struct input_absinfo *)arg;
    memset(target, 0, sizeof(struct input_absinfo));
    target->value = 0; // todo
    target->minimum = 0;
    target->maximum = framebufferHeight;
    ret = 0;
    break;
  }
  case 0x18: // EVIOCGKEY()
    ret = MIN(96, size);
    break;
  case 0x19: // EVIOCGLED()
    ret = MIN(8, size);
    break;
  case 0x1b: // EVIOCGSW()
    ret = MIN(8, size);
    break;
  }

  return ret;
}

void initiateMouse() {
  mouseEvent = devInputEventSetup("PS/2 Mouse");
  // below is optional (vmware didn't do it)
  // mouseEvent->properties = INPUT_PROP_POINTER;
  mouseEvent->inputid.bustype = 0x05;   // BUS_PS2
  mouseEvent->inputid.vendor = 0x045e;  // Microsoft
  mouseEvent->inputid.product = 0x00b4; // Generic MS Mouse
  mouseEvent->inputid.version = 0x0100; // Basic MS Version
  mouseEvent->eventBit = mouseEventBit;

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
