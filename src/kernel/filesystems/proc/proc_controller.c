#include <bootloader.h>
#include <caching.h>
#include <dents.h>
#include <malloc.h>
#include <proc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

// todo: lay this out a better way

Fakefs rootProc = {0};

size_t meminfoRead(OpenFile *fd, uint8_t *out, size_t limit) {
  char   buff[1024] = {0};
  size_t allocated = physical.allocatedSizeInBlocks * BLOCK_SIZE / 1024;
  size_t total = bootloader.mmTotal / 1024;
  size_t free = total - allocated;

  size_t cached = cachingInfoBlocks() * BLOCK_SIZE / 1024;
  size_t available = free + cached;

  size_t length = snprintf(buff, 1024,
                           "%-15s %10lu kB\n"
                           "%-15s %10lu kB\n"
                           "%-15s %10lu kB\n"
                           "%-15s %10lu kB\n",
                           "MemTotal:", total, "MemFree:", free,
                           "MemAvailable:", available, "Cached:", cached);

  size_t toCopy = MIN(length - fd->pointer, limit);
  memcpy(out, buff, toCopy);
  fd->pointer += toCopy;
  return toCopy;
}
VfsHandlers handleMeminfo = {
    .read = meminfoRead, .seek = fsSimpleSeek, .stat = fakefsFstat};

size_t uptimeRead(OpenFile *fd, uint8_t *out, size_t limit) {
  char   buff[1024] = {0};
  size_t secs = timerTicks / 1000;
  int    msFirstTwo = (timerTicks % 1000) / 10;

  size_t length = snprintf(buff, 1024, "%ld.%02d 0.00\n", secs, msFirstTwo);

  size_t toCopy = MIN(length - fd->pointer, limit);
  memcpy(out, buff, toCopy);
  fd->pointer += toCopy;
  return toCopy;
}
VfsHandlers handleUptime = {
    .read = uptimeRead, .seek = fsSimpleSeek, .stat = fakefsFstat};

size_t sRead(OpenFile *fd, uint8_t *out, size_t limit) {
  int   pid = (int)(size_t)fd->dir;
  Task *target = taskGet(pid);
  assert(target);
  size_t toCopy = MIN(target->cmdlineLen - fd->pointer, limit);
  memcpy(out, &target->cmdline[fd->pointer], toCopy);
  fd->pointer += toCopy;
  return toCopy;
}

size_t procEachOpen(char *filename, int flags, int mode, OpenFile *fd,
                    char **symlinkResolve) {
  char *badFn = strdup(filename);
  int   len = strlength(badFn);
  int   lastPos = 0;
  for (int i = 0; i < len; i++) {
    if (badFn[i] == '/') {
      lastPos = i;
      badFn[i] = '\0';
    }
  }
  char *decisionStr = &badFn[lastPos + 1];
  char *pidStr = &badFn[1];
  for (int i = 0; i < (lastPos - 1); i++) { // -1 for the start /
    if (!isdigit(pidStr[i])) {
      free(badFn);
      return ERR(ENOENT);
    }
  }
  int pid = atoi(pidStr);
  if (!strEql(decisionStr, "cmdline")) {
    free(badFn);
    return ERR(ENOENT);
  }
  free(badFn);

  fd->dir = (void *)(size_t)pid;
  return 0;
}

VfsHandlers handleProcEach = {
    .read = sRead, .stat = fakefsFstat, .open = procEachOpen};

void procSetup() {
  fakefsAddFile(&rootProc, rootProc.rootFile, "meminfo", 0,
                S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, &handleMeminfo);
  fakefsAddFile(&rootProc, rootProc.rootFile, "uptime", 0,
                S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, &handleUptime);
  FakefsFile *id =
      fakefsAddFile(&rootProc, rootProc.rootFile, "*", 0,
                    S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH, &fakefsNoHandlers);
  fakefsAddFile(&rootProc, id, "*", 0, S_IFREG | S_IRUSR | S_IRGRP | S_IROTH,
                &handleProcEach);
}

size_t procGetdents64(OpenFile *fd, struct linux_dirent64 *start,
                      unsigned int hardlimit) {
  // todo: fakefs' getdents64() uses ->tmp1 instead of ->pointer
  size_t read = fakefsGetDents64(fd, start, hardlimit);
  if (read)
    return read;

  size_t initPtr = fd->pointer;

  struct linux_dirent64 *dirp = (struct linux_dirent64 *)start;
  size_t                 allocatedlimit = 0;

  spinlockCntReadAcquire(&TASK_LL_MODIFY);
  char  filename[32] = {0};
  Task *browse = firstTask;
  while (browse) {
    // id == tgid so we don't have duplicates. todo thread killing elsewhere
    if (browse->id == browse->tgid && browse->state != TASK_STATE_DEAD &&
        browse->tgid > initPtr) {
      size_t out = snprintf(filename, 32, "%d", browse->tgid);
      assert(out > 0 && out <= 32); // bounds
      DENTS_RES res = dentsAdd(start, &dirp, &allocatedlimit, hardlimit,
                               filename, out, 999 + browse->tgid, 0);
      // todo: type ^^^

      if (res == DENTS_NO_SPACE) {
        allocatedlimit = ERR(EINVAL);
        goto cleanup;
      } else if (res == DENTS_RETURN)
        goto cleanup;

      fd->pointer = browse->tgid;
    }
    browse = browse->next;
  }

cleanup:
  spinlockCntReadRelease(&TASK_LL_MODIFY);
  return allocatedlimit;
}

VfsHandlers procRootHandlers = {.getdents64 = procGetdents64};

bool procMount(MountPoint *mount) {
  // install handlers
  mount->handlers = &fakefsHandlers;
  mount->stat = fakefsStat;
  mount->lstat = fakefsLstat;

  mount->fsInfo = malloc(sizeof(FakefsOverlay));
  memset(mount->fsInfo, 0, sizeof(FakefsOverlay));
  FakefsOverlay *proc = (FakefsOverlay *)mount->fsInfo;

  proc->fakefs = &rootProc;
  if (!rootProc.rootFile) {
    fakefsSetupRoot(&rootProc.rootFile);
    procSetup();
  }
  rootProc.rootFile->handlers = &procRootHandlers;

  return true;
}
