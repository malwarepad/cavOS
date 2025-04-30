#include <apic.h>
#include <console.h>
#include <kb.h>
#include <paging.h>
#include <task.h>

#include <linux.h>
#include <system.h>

// Very bare bones, and basic keyboard driver
// Copyright (C) 2024 Panagiotis

char characterTable[] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-',  '=',  0,    9,    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o',  'p',  '[',  ']',  0,    0,    'a',  's',  'd',  'f',  'g',  'h',
    'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '/',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C,
};

char shiftedCharacterTable[] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_',  '+',  0,    9,    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  0,    0,    'A',  'S',  'D',  'F',  'G',  'H',
    'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '?',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C,
};

const uint8_t evdevTable[89] = {
    0,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LEFTBRACE,
    KEY_RIGHTBRACE,
    KEY_ENTER,
    KEY_LEFTCTRL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_GRAVE,
    KEY_LEFTSHIFT,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_DOT,
    KEY_SLASH,
    KEY_RIGHTSHIFT,
    KEY_KPASTERISK,
    KEY_LEFTALT,
    KEY_SPACE,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_NUMLOCK,
    KEY_SCROLLLOCK,
    KEY_KP7,
    KEY_UP, // KEY_KP8
    KEY_KP9,
    KEY_KPMINUS,
    KEY_LEFT, // KEY_KP4
    KEY_KP5,
    KEY_RIGHT, // KEY_KP6
    KEY_KPPLUS,
    KEY_KP1,
    KEY_DOWN, // KEY_KP2
    KEY_KP3,
    KEY_KP0,
    KEY_DELETE, // KEY_KPDOT
    0,
    0,
    0,
    KEY_F11,
    KEY_F12,
};

// it's just a bitmap to accurately track press/release/repeat ops
#define EVDEV_INTERNAL_SIZE                                                    \
  (DivRoundUp(sizeof(evdevTable) / sizeof(evdevTable[0]), 8))
uint8_t evdevInternal[EVDEV_INTERNAL_SIZE] = {0};

uint8_t lastPressed = 0;

DevInputEvent *kbEvent;

void kbEvdevGenerate(uint8_t raw) {
  uint8_t index = 0;
  bool    clicked = false;
  if (raw <= 0x58) {
    clicked = true;
    index = raw;
  } else if (raw <= 0xD8) {
    clicked = false;
    index = raw - 0x80;
  } else
    return;

  if (index > 88)
    return;
  uint8_t evdevCode = evdevTable[index];
  if (!evdevCode)
    return;

  bool oldstate = bitmapGenericGet(evdevInternal, index);
  if (!oldstate && clicked) {
    // was not clicked previously, now clicked (click)
    inputGenerateEvent(kbEvent, EV_KEY, evdevCode, 1);
    lastPressed = evdevCode;
  } else if (oldstate && clicked) {
    // was clicked previously, now clicked (repeat)
    if (evdevCode != lastPressed)
      return; // no need to re-set it on the bitmap
    inputGenerateEvent(kbEvent, EV_KEY, evdevCode, 2);
  } else if (oldstate && !clicked) {
    // was clicked previously, now not clicked (release)
    inputGenerateEvent(kbEvent, EV_KEY, evdevCode, 0);
  }
  inputGenerateEvent(kbEvent, EV_SYN, SYN_REPORT, 0);

  bitmapGenericSet(evdevInternal, index, clicked);
}

bool shifted = false;
bool capsLocked = false;

char    *kbBuff = 0;
uint32_t kbCurr = 0;
uint32_t kbMax = 0;
uint32_t kbTaskId = 0;

uint8_t kbRead() {
  while (!(inportb(0x64) & 1))
    ;

  return inportb(0x60);
}

void kbWrite(uint16_t port, uint8_t value) {
  while (inportb(0x64) & 2)
    ;

  outportb(port, value);
}

char handleKbEvent() {
  // if (inportb(0x64) & 0x1) {
  uint8_t scanCode = kbRead();
  kbEvdevGenerate(scanCode);

  /* No, I will not fix/improve the rest, idc about the kernel shell */

  // Shift checks
  if (shifted == 1 && scanCode & 0x80) {
    if ((scanCode & 0x7F) == 42) // & 0x7F clears the release
    {
      shifted = 0;
      return 0;
    }
  }

  if (scanCode < sizeof(characterTable) && !(scanCode & 0x80)) {
    char character = (shifted || capsLocked) ? shiftedCharacterTable[scanCode]
                                             : characterTable[scanCode];

    if (character != 0) { // Normal char
      return character;
    }

    switch (scanCode) {
    case SCANCODE_ENTER:
      return CHARACTER_ENTER;
      break;
    case SCANCODE_BACK:
      return CHARACTER_BACK;
      break;
    case SCANCODE_SHIFT:
      shifted = 1;
      break;
    case SCANCODE_CAPS:
      capsLocked = !capsLocked;
      break;
    }
  }
  // }

  return 0;
}

// used by the kernel atm
uint32_t readStr(char *buffstr) {
  while (kbIsOccupied())
    ;
  bool res = kbTaskRead(KERNEL_TASK_ID, buffstr, 1024, false);
  if (!res)
    return 0;

  Task *task = taskGet(KERNEL_TASK_ID);
  if (!task)
    return 0;

  while (kbBuff) {
  }
  uint32_t ret = task->tmpRecV;
  buffstr[ret] = '\0';
  return ret;
}

bool kbTaskRead(uint32_t taskId, char *buff, uint32_t limit,
                bool changeTaskState) {
  while (kbIsOccupied())
    ;
  Task *task = taskGet(taskId);
  if (!task)
    return false;

  kbBuff = buff;
  kbCurr = 0;
  kbMax = limit;
  kbTaskId = taskId;

  if (changeTaskState)
    task->state = TASK_STATE_WAITING_INPUT;
  return true;
}

void kbReset() {
  kbBuff = 0;
  kbCurr = 0;
  kbMax = 0;
  kbTaskId = 0;
}

size_t kbEventBit(OpenFile *fd, uint64_t request, void *arg) {
  size_t number = _IOC_NR(request);
  size_t size = _IOC_SIZE(request);

  size_t ret = ERR(ENOENT);
  switch (number) {
  case 0x20: {
    size_t out = (1 << EV_SYN) | (1 << EV_KEY);
    ret = MIN(sizeof(size_t), size);
    memcpy(arg, &out, ret);
    break;
  }
  case (0x20 + EV_SW):
  case (0x20 + EV_MSC):
  case (0x20 + EV_SND):
  case (0x20 + EV_LED):
  case (0x20 + EV_REL):
  case (0x20 + EV_ABS): {
    ret = MIN(sizeof(size_t), size);
    break;
  }
  case (0x20 + EV_FF): {
    ret = MIN(16, size);
    break;
  }
  case (0x20 + EV_KEY): {
    uint8_t map[96] = {0};
    for (int i = KEY_ESC; i <= KEY_MENU; i++)
      bitmapGenericSet(map, i, true);
    ret = MIN(96, size);
    memcpy(arg, map, ret);
    break;
  }
  case 0x18: {             // EVIOCGKEY()
    uint8_t map[96] = {0}; // NO idea what these do
    // bitmapGenericSet(map, KEY_ENTER, true);
    // bitmapGenericSet(map, KEY_RIGHTSHIFT, true);
    ret = MIN(96, size);
    memcpy(arg, map, ret);
    break;
  }
  case 0x19: // EVIOCGLED()
    ret = MIN(8, size);
    break;
  case 0x1b: // EVIOCGSW()
    ret = MIN(8, size);
    break;
  }

  return ret;
}

void initiateKb() {
  kbEvent = devInputEventSetup("PS/2 Keyboard");
  kbEvent->inputid.bustype = 0x05;   // BUS_PS2
  kbEvent->inputid.vendor = 0x045e;  // Microsoft
  kbEvent->inputid.product = 0x0001; // Generic MS Keyboard
  kbEvent->inputid.version = 0x0100; // Basic MS Version
  kbEvent->eventBit = kbEventBit;

  uint8_t targIrq = ioApicRedirect(1, false);
  registerIRQhandler(targIrq, kbIrq);
  kbReset();
  kbWrite(0x64, 0xae);
  inportb(0x60);
}

void kbFinaliseStream() {
  Task *task = taskGet(kbTaskId);
  if (task) {
    task->tmpRecV = kbCurr;
    task->state = TASK_STATE_READY;
  }
  kbReset();
}

void kbChar(Task *task, char out) {
  if (task->term.c_lflag & ECHO)
    printfch(out);
  if (kbCurr < kbMax)
    kbBuff[kbCurr++] = out;
  if (!(task->term.c_lflag & ICANON))
    kbFinaliseStream();
}

void kbIrq() {
  char out = handleKbEvent();
  if (!kbBuff || !out || !tasksInitiated)
    return;

  Task *task = taskGet(kbTaskId);

  switch (out) {
  case CHARACTER_ENTER:
    // kbBuff[kbCurr] = '\0';
    if (task->term.c_lflag & ICANON)
      kbFinaliseStream();
    else
      kbChar(task, out);
    break;
  case CHARACTER_BACK:
    if (task->term.c_lflag & ICANON && kbCurr > 0) {
      printfch('\b');
      kbCurr--;
      kbBuff[kbCurr] = 0;
    } else if (!(task->term.c_lflag & ICANON))
      kbChar(task, out);
    break;
  default:
    kbChar(task, out);
    break;
  }
}

bool kbIsOccupied() { return !!kbBuff; }
