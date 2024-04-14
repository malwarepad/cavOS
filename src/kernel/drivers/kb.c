#include <kb.h>
#include <paging.h>
#include <task.h>

#include <system.h>

// Very bare bones, and basic keyboard driver
// Copyright (C) 2024 Panagiotis

char characterTable[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-',  '=',  0,    0x09, 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
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
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_',  '+',  0,    0x09, 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
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
  bool res = kbTaskRead(KERNEL_TASK_ID, buffstr, 1024, false);
  if (!res)
    return 0;

  uint32_t ret = 0;
  while (kbBuff) {
    ret = kbCurr;
  }
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

void initiateKb() {
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

void kbIrq() {
  char out = handleKbEvent();
  if (!kbBuff || !out || !tasksInitiated)
    return;

  void *pagedirOld = GetPageDirectory();
  Task *task = taskGet(kbTaskId);
  if (task)
    ChangePageDirectory(task->pagedir);

  switch (out) {
  case CHARACTER_ENTER:
    kbBuff[kbCurr] = '\0';
    kbFinaliseStream();
    break;
  case CHARACTER_BACK:
    if (kbCurr > 0) {
      printfch('\b');
      kbCurr--;
      kbBuff[kbCurr + 1] = 0;
      kbBuff[kbCurr] = 0;
    }
    break;
  default:
    printfch(out);
    kbBuff[kbCurr++] = out;
    break;
  }

  if (task)
    ChangePageDirectory(pagedirOld);
}

bool kbIsOccupied() { return !!kbBuff; }
