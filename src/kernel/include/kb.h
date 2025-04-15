#include "dev.h"
#include "util.h"

#ifndef KB_H
#define KB_H

#define SCANCODE_ENTER 28
#define SCANCODE_BACK 14
#define SCANCODE_SHIFT 42
#define SCANCODE_CAPS 58
#define SCANCODE_UP 0x48

#define CHARACTER_ENTER '\n'
#define CHARACTER_BACK '\b'

uint32_t readStr(char *buffstr);
void     initiateKb();
void     kbIrq();
bool     kbTaskRead(uint32_t taskId, char *buff, uint32_t limit,
                    bool changeTaskState);
bool     kbIsOccupied();

DevInputEvent *kbEvent;

#endif
