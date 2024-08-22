#include <dev.h>
#include <malloc.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

Fakefs rootDev = {0};

void devSetup() {
  fakefsAddFile(&rootDev, rootDev.rootFile, "stdin", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &stdio);
  fakefsAddFile(&rootDev, rootDev.rootFile, "stdout", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &stdio);
  fakefsAddFile(&rootDev, rootDev.rootFile, "stderr", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &stdio);
  fakefsAddFile(&rootDev, rootDev.rootFile, "tty", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &stdio);
  fakefsAddFile(&rootDev, rootDev.rootFile, "fb0", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &fb0);
  fakefsAddFile(&rootDev, rootDev.rootFile, "null", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &handleNull);
}

bool devMount(MountPoint *mount) {
  // install handlers
  mount->handlers = &fakefsHandlers;
  mount->stat = fakefsStat;
  mount->lstat = fakefsLstat;

  mount->fsInfo = malloc(sizeof(FakefsOverlay));
  memset(mount->fsInfo, 0, sizeof(FakefsOverlay));
  FakefsOverlay *dev = (FakefsOverlay *)mount->fsInfo;

  dev->fakefs = &rootDev;
  if (!rootDev.rootFile) {
    fakefsSetupRoot(&rootDev.rootFile);
    devSetup();
  }

  return true;
}
