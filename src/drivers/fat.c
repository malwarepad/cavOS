#include "../../include/fat.h"
#include "../../include/memdefs.h"

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1

typedef uint8 bool;

#define false       0
#define true        1

#define NULL        ((void*)0)
#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

typedef struct 
{
    uint8 BootJumpInstruction[3];
    uint8 OemIdentifier[8];
    uint16 BytesPerSector;
    uint8 SectorsPerCluster;
    uint16 ReservedSectors;
    uint8 FatCount;
    uint16 DirEntryCount;
    uint16 TotalSectors;
    uint8 MediaDescriptorType;
    uint16 SectorsPerFat;
    uint16 SectorsPerTrack;
    uint16 Heads;
    uint32 HiddenSectors;
    uint32 LargeSectorCount;

    // extended boot record
    uint8 DriveNumber;
    uint8 _Reserved;
    uint8 Signature;
    uint32 VolumeId;          // serial number, value doesn't matter
    uint8 VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8 SystemId[8];

    // ... we don't care about code ...

} FAT_BootSector;

typedef struct
{
    uint8 Buffer[SECTOR_SIZE];
    FAT_File Public;
    bool Opened;
    uint32 FirstCluster;
    uint32 CurrentCluster;
    uint32 CurrentSectorInCluster;

} FAT_FileData;

typedef struct
{
    union
    {
        FAT_BootSector BootSector;
        uint8 BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

} FAT_Data;

static FAT_Data* g_Data;
static uint8* g_Fat = NULL;
static uint32 g_DataSectionLba;

void* memcpy(void* dst, const void* src, uint16 num)
{
    uint8* u8Dst = (uint8 *)dst;
    const uint8* u8Src = (const uint8 *)src;

    for (uint16 i = 0; i < num; i++)
        u8Dst[i] = u8Src[i];

    return dst;
}

void* memset(void* ptr, int value, uint16 num)
{
    uint8* u8Ptr = (uint8 *)ptr;

    for (uint16 i = 0; i < num; i++)
        u8Ptr[i] = (uint8)value;

    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, uint16 num)
{
    const uint8* u8Ptr1 = (const uint8 *)ptr1;
    const uint8* u8Ptr2 = (const uint8 *)ptr2;

    for (uint16 i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}

bool islower(char chr)
{
    return chr >= 'a' && chr <= 'z';
}

char toupper(char chr)
{
    return islower(chr) ? (chr - 'a' + 'A') : chr;
}

const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

bool FAT_ReadBootSector(DISK* disk)
{
    return DISK_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk)
{
    return DISK_ReadSectors(disk, g_Data->BS.BootSector.ReservedSectors, g_Data->BS.BootSector.SectorsPerFat, g_Fat);
}

bool FAT_Initialize(DISK* disk)
{
    g_Data = (FAT_Data*)MEMORY_FAT_ADDR;

    // read boot sector
    if (!FAT_ReadBootSector(disk))
    {
        // todo: printf("FAT: read boot sector failed\r\n");
        return false;
    }

    // read FAT
    g_Fat = (uint8*)g_Data + sizeof(FAT_Data);
    uint32 fatSize = g_Data->BS.BootSector.BytesPerSector * g_Data->BS.BootSector.SectorsPerFat;
    if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE)
    {
        // todo: printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }

    if (!FAT_ReadFat(disk))
    {
        // todo: printf("FAT: read FAT failed\r\n");
        return false;
    }

    // open root directory file
    uint32 rootDirLba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
    uint32 rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;

    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    g_Data->RootDirectory.Opened = true;
    g_Data->RootDirectory.FirstCluster = rootDirLba;
    g_Data->RootDirectory.CurrentCluster = rootDirLba;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer))
    {
        // todo: printf("FAT: read root directory failed\r\n");
        return false;
    }

    // calculate data section
    uint32 rootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSectors;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;

    return true;
}

uint32 FAT_ClusterToLba(uint32 cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        // todo: printf("FAT: out of file handles\r\n");
        return false;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        // todo: printf("FAT: read error\r\n");
        return false;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32 FAT_NextCluster(uint32 currentCluster)
{    
    uint32 fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16*)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16*)(g_Fat + fatIndex)) >> 4;
}

uint32 FAT_Read(DISK* disk, FAT_File* file, uint32 byteCount, void* dataOut)
{
    // get file data
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_Data->RootDirectory 
        : &g_Data->OpenedFiles[file->Handle];

    uint8* u8DataOut = (uint8*)dataOut;

    // don't read past the end of the file
    if (!fd->Public.IsDirectory) 
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32 leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32 take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        // // todo: printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // Special handling for root directory
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                // read next sector
                if (!DISK_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    // todo: printf("FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                // calculate next cluster & sector to read
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8)
                {
                    // Mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    // todo: printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8*)dataOut;
}

bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else
    {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}

bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char fatName[12];
    FAT_DirectoryEntry entry;

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != NULL)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (memcmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }
    
    return false;
}

FAT_File* FAT_Open(DISK* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data->RootDirectory.Public;

    while (*path) {
        // extract next file name from path
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlength(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }
        
        // find directory entry in current directory
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            FAT_Close(current);

            // check if directory
            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0)
            {
                // todo: printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(disk, &entry);
        }
        else
        {
            FAT_Close(current);

            // todo: printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }

    return current;
}