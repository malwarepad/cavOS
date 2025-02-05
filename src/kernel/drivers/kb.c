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
};

char shiftedCharacterTable[] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_',  '+',  0,    9,    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  0,    0,    'A',  'S',  'D',  'F',  'G',  'H',
    'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,
};

bool shifted = false;
bool capsLocked = false;
bool ctrlPressed = false;
bool altPressed = false;

char    *kbBuff = 0;
uint32_t kbCurr = 0;
uint32_t kbMax = 0;
uint32_t kbTaskId = 0;

uint8_t kbRead() {
    while (!(inportb(0x64) & 1));
    return inportb(0x60);
}

void kbWrite(uint16_t port, uint8_t value) {
    while (inportb(0x64) & 2);
    outportb(port, value);
}

char handleKbEvent() {
    uint8_t scanCode = kbRead();

    // Handle Ctrl key
    if (scanCode == 0x1D) {  // Left Ctrl pressed
        ctrlPressed = true;
        return 0;
    } else if (scanCode == 0x9D) {  // Left Ctrl released
        ctrlPressed = false;
        return 0;
    }

    // Handle Alt key
    if (scanCode == 0x38) {  // Left Alt pressed
        altPressed = true;
        return 0;
    } else if (scanCode == 0xB8) {  // Left Alt released
        altPressed = false;
        return 0;
    }

    // Shift checks
    if (shifted == 1 && scanCode & 0x80) {
        if ((scanCode & 0x7F) == 42) { // & 0x7F clears the release bit
            shifted = 0;
            return 0;
        }
    }

    if (scanCode < sizeof(characterTable) && !(scanCode & 0x80)) {
        char character = (shifted || capsLocked) ? shiftedCharacterTable[scanCode]
                                                 : characterTable[scanCode];

        if (character != 0) {
            return character;
        }

        switch (scanCode) {
            case SCANCODE_ENTER:
                return CHARACTER_ENTER;
            case SCANCODE_BACK:
                return CHARACTER_BACK;
            case SCANCODE_SHIFT:
                shifted = true;
                break;
            case SCANCODE_CAPS:
                capsLocked = !capsLocked;
                break;
        }
    }

    return 0;
}

uint32_t readStr(char *buffstr) {
    while (kbIsOccupied());
    bool res = kbTaskRead(KERNEL_TASK_ID, buffstr, 1024, false);
    if (!res) return 0;

    Task *task = taskGet(KERNEL_TASK_ID);
    if (!task) return 0;

    while (kbBuff);
    uint32_t ret = task->tmpRecV;
    buffstr[ret] = '\0';
    return ret;
}

bool kbTaskRead(uint32_t taskId, char *buff, uint32_t limit, bool changeTaskState) {
    while (kbIsOccupied());
    Task *task = taskGet(taskId);
    if (!task) return false;

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

    // Example: Handle Ctrl + C (Common for terminating processes)
    if (ctrlPressed && out == 'c') {
        printf("CTRL+C Pressed! Interrupt signal.\n");
        return;
    }

    // Example: Handle Alt + X
    if (altPressed && out == 'x') {
        printf("ALT+X detected!\n");
        return;
    }

    switch (out) {
        case CHARACTER_ENTER:
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
