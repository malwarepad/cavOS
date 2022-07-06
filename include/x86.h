#include "vga.h"
#include "util.h"
#include "multiboot.h"

#ifndef X86_H
#define X86_H

extern uint8 x86_Disk_Reset(uint8 drive);

extern uint8 x86_Disk_Read(uint8 drive,uint16 cylinder,uint16 sector,uint16 head,uint8 count, void* dataOut);

extern uint8 x86_Disk_GetDriveParams(uint8 drive,uint8* driveTypeOut,uint16* cylindersOut,uint16* sectorsOut,uint16* headsOut);

#endif
