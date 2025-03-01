#include <caching.h>
#include <system.h>
#include <vfs.h>

size_t cachingInfoBlocks() {
  size_t ret = 0;

  // filesystem cache
  MountPoint *fs = firstMountPoint;
  while (fs) {
    ret += fs->blocksCached;
    fs = fs->next;
  }

  return ret;
}
