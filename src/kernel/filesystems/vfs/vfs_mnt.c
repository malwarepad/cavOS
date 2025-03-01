#include <dev.h>
#include <disk.h>
#include <ext2.h>
#include <fat32.h>
#include <linked_list.h>
#include <malloc.h>
#include <proc.h>
#include <string.h>
#include <sys.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// Different mount point separation
// Copyright (C) 2024 Panagiotis

// todo: add LL lock to mountPoints

bool fsUnmount(MountPoint *mnt) {
  debugf("[vfs] Tried to unmount!\n");
  panic();
  LinkedListUnregister((void **)&firstMountPoint, mnt);

  // todo!
  // switch (mnt->filesystem) {
  // case FS_FATFS:
  //   f_unmount("/");
  //   break;
  // }

  free(mnt->prefix);
  free(mnt);

  return true;
}

bool isFat(mbr_partition *mbr) {
  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, mbr->lba_first_sector, 1);

  bool ret = (rawArr[66] == 0x28 || rawArr[66] == 0x29);

  free(rawArr);

  return ret;
}

bool isExt2(mbr_partition *mbr) { return mbr->type == 0x83; }

// prefix MUST end with '/': /mnt/handle/
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition) {
  MountPoint *mount = (MountPoint *)LinkedListAllocate(
      (void **)&firstMountPoint, sizeof(MountPoint));

  uint32_t strlen = strlength(prefix);
  mount->prefix = (char *)(malloc(strlen + 1));
  memcpy(mount->prefix, prefix, strlen);
  mount->prefix[strlen] = '\0'; // null terminate

  mount->disk = disk;
  mount->partition = partition;
  mount->connector = connector;

  bool ret = false;
  switch (connector) {
  case CONNECTOR_AHCI:
    if (!openDisk(disk, partition, &mount->mbr)) {
      fsUnmount(mount);
      return 0;
    }

    if (isFat(&mount->mbr)) {
      mount->filesystem = FS_FATFS;
      ret = fat32Mount(mount);
    } else if (isExt2(&mount->mbr)) {
      mount->filesystem = FS_EXT2;
      ret = ext2Mount(mount);
    }
    break;
  case CONNECTOR_DEV:
    mount->filesystem = FS_DEV;
    ret = devMount(mount);
    break;
  case CONNECTOR_SYS:
    mount->filesystem = FS_SYS;
    ret = sysMount(mount);
    break;
  case CONNECTOR_PROC:
    mount->filesystem = FS_PROC;
    ret = procMount(mount);
    break;
  default:
    debugf("[vfs] Tried to mount with bad connector! id{%d}\n", connector);
    ret = 0;
    break;
  }

  if (!ret) {
    fsUnmount(mount);
    return 0;
  }

  if (!systemDiskInit && strlength(prefix) == 1 && prefix[0] == '/')
    systemDiskInit = true;
  return mount;
}

MountPoint *fsDetermineMountPoint(char *filename) {
  MountPoint *largestAddr = 0;
  uint32_t    largestLen = 0;

  MountPoint *browse = firstMountPoint;
  while (browse) {
    size_t len = strlength(browse->prefix) - 1; // without trailing /
    if (len >= largestLen && memcmp(filename, browse->prefix, len) == 0 &&
        (filename[len] == '/' || filename[len] == '\0')) {
      largestAddr = browse;
      largestLen = len;
    }
    browse = browse->next;
  }

  return largestAddr;
}

// make SURE to free both! also returns non-safe filename, obviously
char *fsResolveSymlink(MountPoint *mnt, char *symlink) {
  int symlinkLength = strlength(symlink);
  int prefixLength = strlength(mnt->prefix) - 1; // remove slash

  char *ret = 0;
  if (symlink[0] == '/') {
    // needs prefixing
    ret = malloc(prefixLength + symlinkLength + 1);
    memcpy(ret, mnt->prefix, prefixLength);
    memcpy(&ret[prefixLength], symlink, symlinkLength);
    ret[prefixLength + symlinkLength] = '\0';
  } else if (symlink[0] == '!') {
    // doesn't need prefixing
    ret = malloc(symlinkLength + 1);
    memcpy(ret, &symlink[1], symlinkLength);
    ret[symlinkLength] = '\0';
  }

  return ret;
}
