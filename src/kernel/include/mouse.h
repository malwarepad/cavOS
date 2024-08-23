#include "types.h"

#ifndef MOUSE_H
#define MOUSE_H

#define MOUSE_PORT 0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT 0x02
#define MOUSE_BBIT 0x01
#define MOUSE_WRITE 0xD4
#define MOUSE_F_BIT 0x20
#define MOUSE_V_BIT 0x08

#define MOUSE_TIMEOUT 100000

void initiateMouse();
void mouseIrq();

#endif
