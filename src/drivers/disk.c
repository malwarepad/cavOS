#include "../../include/disk.h"

uint8 DISK_Initialize(DISK* disk, uint8 driveNumber)
{
    uint8 driveType;
    uint16 cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &sectors, &heads))
        return 0;

    disk->id = driveNumber;
    disk->cylinders = cylinders + 1;
    disk->heads = heads + 1;
    disk->sectors = sectors;

    return 1;
}

void DISK_LBA2CHS(DISK* disk, uint32 lba, uint16* cylinderOut, uint16* sectorOut, uint16* headOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % disk->sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / disk->sectors) / disk->heads;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / disk->sectors) % disk->heads;
}

uint8 DISK_ReadSectors(DISK* disk, uint32 lba, uint8 sectors, void *dataOut)
{
    uint16 cylinder, sector, head;

    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, dataOut))
            return 1;

        x86_Disk_Reset(disk->id);
    }

    return 0;
}

/*
 BSY: a 1 means that the controller is busy executing a command. No register should be accessed (except the digital output register) while this bit is set.
RDY: a 1 means that the controller is ready to accept a command, and the drive is spinning at correct speed..
WFT: a 1 means that the controller detected a write fault.
SKC: a 1 means that the read/write head is in position (seek completed).
DRQ: a 1 means that the controller is expecting data (for a write) or is sending data (for a read). Don't access the data register while this bit is 0.
COR: a 1 indicates that the controller had to correct data, by using the ECC bytes (error correction code: extra bytes at the end of the sector that allows to verify its integrity and, sometimes, to correct errors).
IDX: a 1 indicates the the controller retected the index mark (which is not a hole on hard-drives).
ERR: a 1 indicates that an error occured. An error code has been placed in the error register.
*/

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DRQ 0x08
#define STATUS_DF 0x20
#define STATUS_ERR 0x01

//This is really specific to out OS now, assuming ATA bus 0 master
//Source - OsDev wiki
static void ATA_wait_BSY();
static void ATA_wait_DRQ();
void read_sectors_ATA_PIO(uint32 target_address, uint32 LBA, uint8 sector_count)
{

    ATA_wait_BSY();
    outportb(0x1F6,0xE0 | ((LBA >>24) & 0xF));
    outportb(0x1F2,sector_count);
    outportb(0x1F3, (uint8) LBA);
    outportb(0x1F4, (uint8)(LBA >> 8));
    outportb(0x1F5, (uint8)(LBA >> 16));
    outportb(0x1F7,0x20); //Send the read command

    uint16 *target = (uint16*) target_address;

    for (int j =0;j<sector_count;j++)
    {
        ATA_wait_BSY();
        ATA_wait_DRQ();
        for(int i=0;i<256;i++)
            target[i] = inportb(0x1F0);
        target+=256;
    }
}

void write_sectors_ATA_PIO(uint32 LBA, uint8 sector_count, uint32* bytes)
{
    ATA_wait_BSY();
    outportb(0x1F6,0xE0 | ((LBA >>24) & 0xF));
    outportb(0x1F2,sector_count);
    outportb(0x1F3, (uint8) LBA);
    outportb(0x1F4, (uint8)(LBA >> 8));
    outportb(0x1F5, (uint8)(LBA >> 16));
    outportb(0x1F7,0x30); //Send the write command

    for (int j =0;j<sector_count;j++)
    {
        ATA_wait_BSY();
        ATA_wait_DRQ();
        for(int i=0;i<256;i++)
        {
            outportb(0x1F0, bytes[i]);
        }
    }
}

static void ATA_wait_BSY()   //Wait for bsy to be 0
{
    while(inportb(0x1F7)&STATUS_BSY);
}
static void ATA_wait_DRQ()  //Wait fot drq to be 1
{
    while(!(inportb(0x1F7)&STATUS_RDY));
}
