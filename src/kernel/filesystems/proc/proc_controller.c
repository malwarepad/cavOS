#include <bootloader.h>
#include <caching.h>
#include <malloc.h>
#include <proc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

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
  int   pid = atoi(pidStr);
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
                    S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, &fakefsNoHandlers);
  fakefsAddFile(&rootProc, id, "*", 0, S_IFREG | S_IRUSR | S_IRGRP | S_IROTH,
                &handleProcEach);
}

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

  return true;
}
