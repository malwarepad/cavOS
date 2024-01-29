#include <ata.h>
#include <disk.h>
#include <fat32.h>
#include <fs_controller.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>

bool fsUnmount(MountPoint *mnt) {
  MountPoint *browse = firstMountPoint;
  while (browse) {
    if (browse->next == mnt)
      break;
    browse = browse->next;
  }
  MountPoint *target;
  if (firstMountPoint == mnt) {
    target = firstMountPoint;
    firstMountPoint = target->next;
  } else if (browse) {
    target = browse->next;
    browse->next = target->next;
  } else {
    target = mnt;
  }

  switch (target->filesystem) {
  case FS_FAT32:
    finaliseFat32(target);
    break;
  }

  free(target->prefix);
  free(target);

  return true;
}

// prefix MUST end with '/': /mnt/handle/
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition) {
  MountPoint *browse = firstMountPoint;
  MountPoint *mount = (MountPoint *)malloc(sizeof(MountPoint));
  memset(mount, 0, sizeof(MountPoint));
  while (1) {
    if (browse == 0) {
      // means this is our first one
      firstMountPoint = mount;
      break;
    }
    if (browse->next == 0) {
      // next is non-existent (end of linked list)
      browse->next = mount;
      break;
    }
    browse = browse->next; // cycle
  }

  uint32_t strlen = strlength(prefix);
  mount->prefix = (char *)(malloc(strlen + 1));
  memcpy(mount->prefix, prefix, strlen);
  mount->prefix[strlen] = '\0'; // null terminate

  mount->disk = disk;
  mount->partition = partition;
  mount->connector = connector;

  if (!openDisk(disk, partition, &mount->mbr)) {
    fsUnmount(mount);
    return 0;
  }

  bool ret = false;
  if (isFat32(&mount->mbr)) {
    mount->filesystem = FS_FAT32;
    ret = initiateFat32(mount);
  }

  if (!ret) {
    fsUnmount(mount);
    return 0;
  }

  if (!systemDiskInit && strlength(prefix) == 1 && prefix[0] == '/')
    systemDiskInit = true;
  return mount;
}

OpenFile *fsKernelRegisterNode() {
  OpenFile *target = (OpenFile *)malloc(sizeof(OpenFile));
  memset(target, 0, sizeof(OpenFile));
  OpenFile *browse = firstKernelFile;
  while (1) {
    if (browse == 0) {
      // means this is our first one
      firstKernelFile = target;
      break;
    }
    if (browse->next == 0) {
      // next is non-existent (end of linked list)
      browse->next = target;
      break;
    }
    browse = browse->next;
  }

  return target;
}

OpenFile *fsKernelUnregisterNode(OpenFile *file) {
  OpenFile *browse = firstKernelFile;
  while (browse) {
    if (browse->next == file)
      break;
    browse = browse->next;
  }
  OpenFile *target;
  if (firstKernelFile == file) {
    target = firstKernelFile;
    firstKernelFile = target->next;
  } else if (browse) {
    target = browse->next;
    browse->next = target->next;
  } else {
    target = file;
  }

  return target;
}

OpenFile *fsUserRegisterNode(Task *task) {
  OpenFile *target = (OpenFile *)malloc(sizeof(OpenFile));
  memset(target, 0, sizeof(OpenFile));
  OpenFile *browse = task->firstFile;
  while (1) {
    if (browse == 0) {
      // means this is our first one
      task->firstFile = target;
      break;
    }
    if (browse->next == 0) {
      // next is non-existent (end of linked list)
      browse->next = target;
      break;
    }
    browse = browse->next;
  }

  return target;
}

OpenFile *fsUserNodeFetch(Task *task, int fd) {
  OpenFile *browse = task->firstFile;
  while (browse) {
    if (browse->id == fd)
      break;
    browse = browse->next;
  }
  return browse;
}

OpenFile *fsUserUnregisterNode(Task *task, OpenFile *file) {
  OpenFile *browse = task->firstFile;
  while (browse) {
    if (browse->next == file)
      break;
    browse = browse->next;
  }
  OpenFile *target;
  if (task->firstFile == file) {
    target = task->firstFile;
    task->firstFile = target->next;
  } else if (browse) {
    target = browse->next;
    browse->next = target->next;
  } else {
    target = file;
  }

  return target;
}

bool fsCloseFsSpecific(OpenFile *file) {
  bool res = false;
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    free(file->dir);
    res = true;
    break;
  }

  return res;
}

bool fsOpenFsSpecific(char *filename, MountPoint *mnt, OpenFile *target) {
  bool  res = false;
  char *strippedFilename =
      (char *)((uint32_t)filename + strlength(mnt->prefix) -
               1); // -1 for putting start slash
  switch (mnt->filesystem) {
  case FS_FAT32:
    FAT32_Directory *dir = (FAT32_Directory *)malloc(sizeof(FAT32_Directory));
    target->dir = dir;
    res = fatOpenFile(mnt->fsInfo, dir, strippedFilename);
    break;
  }
  return res;
}

// todo: sanitize filenames
int       openId = 2;
OpenFile *fsOpenGeneric(char *filename, Task *task) {
  OpenFile *target = task ? fsUserRegisterNode(task) : fsKernelRegisterNode();
  target->id = openId++;

  target->pointer = 0;
  target->tmp1 = 0;

  MountPoint *mnt = fsDetermineMountPoint(filename);
  if (!mnt) {
    // no mountpoint for this
    fsKernelUnregisterNode(target);
    free(target);
    return 0;
  }
  target->mountPoint = mnt;

  bool res = fsOpenFsSpecific(filename, mnt, target);

  if (!res) {
    // failed to open
    if (task)
      fsUserUnregisterNode(task, target);
    else
      fsKernelUnregisterNode(target);
    free(target);
    return 0;
  }

  return target;
}

OpenFile *fsKernelOpen(char *filename) { return fsOpenGeneric(filename, 0); }

int fsUserOpen(char *filename, int flags, uint16_t mode) {
  // todo: modes & flags
  OpenFile *file = fsOpenGeneric(filename, currentTask);
  if (!file)
    return -1;

  return file->id;
}

bool fsCloseGeneric(OpenFile *file, Task *task) {
  if (task)
    fsUserUnregisterNode(task, file);
  else
    fsKernelUnregisterNode(file);

  bool res = fsCloseFsSpecific(file);

  free(file);
  return res;
}

bool fsKernelClose(OpenFile *file) { return fsCloseGeneric(file, 0); }

int fsUserClose(int fd) {
  if (fd < 2)
    return -1;
  OpenFile *file = fsUserNodeFetch(currentTask, fd);
  if (!file)
    return -1;
  bool res = fsCloseGeneric(file, currentTask);
  if (res)
    return 1;
  else
    return -1;
}

MountPoint *fsDetermineMountPoint(char *filename) {
  MountPoint *largestAddr = 0;
  uint32_t    largestLen = 0;

  MountPoint *browse = firstMountPoint;
  while (browse) {
    if (strlength(browse->prefix) > largestLen &&
        memcmp(filename, browse->prefix, strlength(browse->prefix)) == 0) {
      largestAddr = browse;
      largestLen = strlength(browse->prefix);
    }
    browse = browse->next;
  }

  return largestAddr;
}

uint32_t fsGetFilesize(OpenFile *file) {
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    return ((FAT32_Directory *)(file->dir))->filesize;
    break;
  }

  return 0;
}

void fsReadFullFile(OpenFile *file, char *out) {
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    readFileContents(file->mountPoint->fsInfo, out, file->dir);
    break;
  }
}
