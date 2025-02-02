#include <bootloader.h>
#include <malloc.h>
#include <proc.h>
#include <system.h>
#include <timer.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

Fakefs rootProc = {0};

size_t meminfoRead(OpenFile *fd, uint8_t *out, size_t limit) {
  char   buff[1024] = {0};
  size_t allocated = physical.allocatedSizeInBlocks / 1024;
  size_t total = bootloader.mmTotal / 1024;
  size_t free = total - allocated;

  size_t length =
      snprintf(buff, 1024,
               "%-15s %10lu kB\n"
               "%-15s %10lu kB\n"
               "%-15s %10lu kB\n",
               "MemTotal:", total, "MemFree:", free, "MemAvailable:", free);

  size_t toCopy = MIN(length - fd->pointer, limit);
  memcpy(out, buff, toCopy);
  fd->pointer += toCopy;
  return toCopy;
}
VfsHandlers handleMeminfo = {.read = meminfoRead, .stat = fakefsFstat};

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
VfsHandlers handleUptime = {.read = uptimeRead, .stat = fakefsFstat};

void procSetup() {
  fakefsAddFile(&rootProc, rootProc.rootFile, "meminfo", 0,
                S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, &handleMeminfo);
  fakefsAddFile(&rootProc, rootProc.rootFile, "uptime", 0,
                S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, &handleUptime);
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
