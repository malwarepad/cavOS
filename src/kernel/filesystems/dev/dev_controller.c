#include <dev.h>
#include <malloc.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

// todo: extremely primitive and bad
size_t randomRead(OpenFile *fd, uint8_t *out, size_t limit) {
  size_t limitDiv = limit / 4;
  int    limitMod = limit % 4;

  uint32_t *outLarge = (uint32_t *)out;
  for (int i = 0; i < limitDiv; i++)
    outLarge[i] = rand();
  for (int i = 0; i < limitMod; i++)
    out[limit - 1 - i] = rand() & 0xFF;
  return limit;
}
VfsHandlers handleRandom = {.read = randomRead, .stat = fakefsFstat};

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
  fakefsAddFile(&rootDev, rootDev.rootFile, "random", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &handleRandom);
  fakefsAddFile(&rootDev, rootDev.rootFile, "urandom", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &handleRandom);

  initiatePtyInterface();
  fakefsAddFile(&rootDev, rootDev.rootFile, "ptmx", 0,
                S_IFCHR | S_IRUSR | S_IWUSR, &handlePtmx);
  FakefsFile *pts =
      fakefsAddFile(&rootDev, rootDev.rootFile, "pts", 0,
                    S_IFDIR | S_IRUSR | S_IWUSR, &fakefsRootHandlers);
  fakefsAddFile(&rootDev, pts, "*", 0, S_IFCHR | S_IRUSR | S_IWUSR, &handlePts);

  inputFakedir =
      fakefsAddFile(&rootDev, rootDev.rootFile, "input", 0,
                    S_IFDIR | S_IRUSR | S_IWUSR, &fakefsRootHandlers);
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
