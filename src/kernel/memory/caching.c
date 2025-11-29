#include <caching.h>
#include <system.h>
#include <vfs.h>

void cachingInfoCb(void *data, void *ctx) {
  MountPoint *fs = data;
  *(size_t *)ctx += fs->blocksCached;
}

size_t cachingInfoBlocks() {
  size_t ret = 0;
  LinkedListTraverse(&dsMountPoint, cachingInfoCb, &ret);
  return ret;
}
