#ifndef FAT_H
#define FAT_H

#include "vga.h"
#include "util.h"
#include "multiboot.h"
#include "x86.h"
#include "disk.h"

typedef struct
{
    uint8 Name[11];
    uint8 Attributes;
    uint8 _Reserved;
    uint8 CreatedTimeTenths;
    uint16 CreatedTime;
    uint16 CreatedDate;
    uint16 AccessedDate;
    uint16 FirstClusterHigh;
    uint16 ModifiedTime;
    uint16 ModifiedDate;
    uint16 FirstClusterLow;
    uint32 Size;
} FAT_DirectoryEntry;

typedef struct
{
    int Handle;
    uint8 IsDirectory;
    uint32 Position;
    uint32 Size;
} FAT_File;

enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

uint8 FAT_Initialize(DISK* disk);
FAT_File* FAT_Open(DISK* disk, const char* path);
uint32 FAT_Read(DISK* disk, FAT_File* file, uint32 byteCount, void* dataOut);
uint8 FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File* file);

#endif
