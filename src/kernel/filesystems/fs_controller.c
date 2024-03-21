#include <disk.h>
#include <fat32.h>
#include <fs_controller.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>

bool fsUnmount(MountPoint *mnt) {
  LinkedListUnregister(&firstMountPoint, mnt);

  switch (mnt->filesystem) {
  case FS_FAT32:
    finaliseFat32(mnt);
    break;
  }

  free(mnt->prefix);
  free(mnt);

  return true;
}

// prefix MUST end with '/': /mnt/handle/
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition) {
  MountPoint *mount =
      (MountPoint *)LinkedListAllocate(&firstMountPoint, sizeof(MountPoint));

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

    if (isFat32(&mount->mbr)) {
      mount->filesystem = FS_FAT32;
      ret = initiateFat32(mount);
    }
    break;
  case CONNECTOR_DUMMY:
    mount->filesystem = FS_TEST;
    ret = 1;
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
    if (strlength(browse->prefix) > largestLen &&
        memcmp(filename, browse->prefix, strlength(browse->prefix)) == 0) {
      largestAddr = browse;
      largestLen = strlength(browse->prefix);
    }
    browse = browse->next;
  }

  return largestAddr;
}

OpenFile *fsKernelRegisterNode() {
  return LinkedListAllocate(&firstKernelFile, sizeof(OpenFile));
}

bool fsKernelUnregisterNode(OpenFile *file) {
  return LinkedListUnregister(&firstKernelFile, file);
}

OpenFile *fsUserRegisterNode(Task *task) {
  return LinkedListAllocate(&task->firstFile, sizeof(OpenFile));
}

bool fsUserUnregisterNode(Task *task, OpenFile *file) {
  return LinkedListUnregister(&task->firstFile, file);
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
  char *strippedFilename = (char *)((size_t)filename + strlength(mnt->prefix) -
                                    1); // -1 for putting start slash
  switch (mnt->filesystem) {
  case FS_FAT32:
    FAT32_Directory *dir = (FAT32_Directory *)malloc(sizeof(FAT32_Directory));
    target->dir = dir;
    res = fatOpenFile(mnt->fsInfo, dir, strippedFilename);
    break;
  case FS_TEST:
    res = 1;
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

uint32_t fsGetFilesize(OpenFile *file) {
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    return ((FAT32_Directory *)(file->dir))->filesize;
    break;
  case FS_TEST:
    return 4096;
    break;
  }

  return 0;
}

uint32_t fsRead(OpenFile *file, char *out, uint32_t limit) {
  uint32_t ret = 0;
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    ret = readFileContents(file, file->mountPoint->fsInfo, out, limit);
    break;
  case FS_TEST:
    memset(out, 'p', limit);
    break;
  }
  return ret;
}

void fsReadFullFile(OpenFile *file, uint8_t *out) {
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    uint32_t read = readFileContents(file, file->mountPoint->fsInfo, out,
                                     fsGetFilesize(file));
    break;
  case FS_TEST:
    fsRead(file, out, fsGetFilesize(file));
    break;
  }
}

#define SEEK_SET 0  // start + offset
#define SEEK_CURR 1 // current + offset
#define SEEK_END 2  // end + offset
int fsUserSeek(uint32_t fd, int offset, int whence) {
  OpenFile *file = fsUserNodeFetch(currentTask, fd);
  int       target = offset;
  if (whence == SEEK_SET)
    target += 0;
  else if (whence == SEEK_CURR)
    target += file->pointer;
  else if (whence == SEEK_END)
    target += fsGetFilesize(file);

  bool ret = false;
  switch (file->mountPoint->filesystem) {
  case FS_FAT32:
    ret = fat32Seek(file, target);
    break;
  }
  if (!ret)
    return -1;
  return target;
}
