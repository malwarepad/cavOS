/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* Declarations of disk functions */
#include "ff.h"     /* Obtains integer types */

/* Definitions of physical drive number for each drive */
#define DEV_RAM 0 /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC 1 /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB 2 /* Example: Map USB MSD to physical drive 2 */

#include <disk.h>
#include <util.h>
#include <vmm.h>
char *strchr(const char *s, int c) {
  while (*s != (char)c && *s != '\0') {
    s++;
  }
  if (*s == (char)c) {
    return (char *)s;
  }
  return 0;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
  DSTATUS stat = 0;
  return stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
  DSTATUS stat = 0;

  return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE  pdrv, /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT  count   /* Number of sectors to read */
) {
  DRESULT res;
  getDiskBytes(buff, sector, count);
  res = RES_OK;

  return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff,   /* Data to be written */
                   LBA_t       sector, /* Start sector in LBA */
                   UINT        count   /* Number of sectors to write */
) {
  DRESULT res = RES_OK;
  setDiskBytes(buff, sector, count);

  return res;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE  pdrv, /* Physical drive nmuber (0..) */
                   BYTE  cmd,  /* Control code */
                   void *buff  /* Buffer to send/receive control data */
) {
  DRESULT res = RES_OK;

  switch (cmd) {
  case CTRL_SYNC:
    // writes are synced, nothing to do lol
    break;

  default:
    debugf("[fatfs] UNIMPLEMENTED! Tried to ioctl: pdrv{%x} cmd{%x} "
           "buff{%lx}\n",
           pdrv, cmd, buff);
    res = RES_PARERR;
    break;
  }

  return res;
}

#include <malloc.h>
#include <rtc.h>
uint32_t get_fattime() {
  RTC *rtc = malloc(sizeof(RTC));
  readFromCMOS(rtc);
  uint32_t out = (DWORD)(rtc->year - 80) << 25 | (DWORD)(rtc->month + 1) << 21 |
                 (DWORD)rtc->day << 16 | (DWORD)rtc->hour << 11 |
                 (DWORD)rtc->minute << 5 | (DWORD)rtc->second >> 1;
  free(rtc);
  return out;
}
