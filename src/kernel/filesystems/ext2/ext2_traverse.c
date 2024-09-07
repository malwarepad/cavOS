#include <ext2.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <timer.h>
#include <util.h>

Ext2Inode *ext2InodeFetch(Ext2 *ext2, size_t inode) {
  uint32_t group = INODE_TO_BLOCK_GROUP(ext2, inode);
  uint32_t index = INODE_TO_INDEX(ext2, inode);

  size_t leftovers = index * ext2->inodeSize;
  size_t leftoversLba = leftovers / SECTOR_SIZE;
  size_t leftoversRem = leftovers % SECTOR_SIZE;

  // large enough just in case
  size_t len = DivRoundUp(ext2->inodeSize * 4, SECTOR_SIZE) * SECTOR_SIZE;
  size_t lba =
      BLOCK_TO_LBA(ext2, 0, ext2->bgdts[group].inode_table) + leftoversLba;

  uint8_t *buf = (uint8_t *)malloc(len);
  getDiskBytes(buf, lba, len / SECTOR_SIZE);
  Ext2Inode *tmp = (Ext2Inode *)(buf + leftoversRem);

  Ext2Inode *ret = (Ext2Inode *)malloc(ext2->inodeSize);
  memcpy(ret, tmp, ext2->inodeSize);

  free(buf);
  return ret;
}

// IMPORTANT! Remember to manually set the lock **before** calling
void ext2InodeModifyM(Ext2 *ext2, size_t inode, Ext2Inode *target) {
  uint32_t group = INODE_TO_BLOCK_GROUP(ext2, inode);
  uint32_t index = INODE_TO_INDEX(ext2, inode);

  size_t leftovers = index * ext2->inodeSize;
  size_t leftoversLba = leftovers / SECTOR_SIZE;
  size_t leftoversRem = leftovers % SECTOR_SIZE;

  // large enough just in case
  size_t len = DivRoundUp(ext2->inodeSize * 4, SECTOR_SIZE) * SECTOR_SIZE;
  size_t lba =
      BLOCK_TO_LBA(ext2, 0, ext2->bgdts[group].inode_table) + leftoversLba;

  uint8_t *buf = (uint8_t *)malloc(len);
  getDiskBytes(buf, lba, len / SECTOR_SIZE);
  Ext2Inode *tmp = (Ext2Inode *)(buf + leftoversRem);
  memcpy(tmp, target, sizeof(Ext2Inode));
  setDiskBytes(buf, lba, len / SECTOR_SIZE);

  free(buf);
}

uint32_t ext2Traverse(Ext2 *ext2, size_t initInode, char *search,
                      size_t searchLength) {
  uint32_t   ret = 0;
  Ext2Inode *ino = ext2InodeFetch(ext2, initInode);
  uint8_t   *names = (uint8_t *)malloc(ext2->blockSize);

  Ext2LookupControl control = {0};
  size_t            blockNum = 0;

  ext2BlockFetchInit(ext2, &control);

  int blocksContained = DivRoundUp(ino->size, ext2->blockSize);
  for (int i = 0; i < blocksContained; i++) {
    size_t block = ext2BlockFetch(ext2, ino, &control, blockNum);
    blockNum++;
    if (!block)
      break;
    Ext2Directory *dir = (Ext2Directory *)names;

    getDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                 ext2->blockSize / SECTOR_SIZE);

    while (((size_t)dir - (size_t)names) < ext2->blockSize) {
      if (!dir->inode)
        continue;
      if (dir->filenameLength == searchLength &&
          memcmp(dir->filename, search, searchLength) == 0) {
        ret = dir->inode;
        goto cleanup;
      }
      dir = (void *)((size_t)dir + dir->size);
    }
  }

cleanup:
  ext2BlockFetchCleanup(&control);
  free(ino);
  free(names);
  return ret;
}

uint32_t ext2TraversePath(Ext2 *ext2, char *path, size_t initInode, bool follow,
                          char **symlinkResolve) {
  uint32_t curr = initInode;
  size_t   len = strlength(path);

  if (len == 1) // meaning it's trying to open /
    return 2;

  int lastslash = 0;
  for (int i = 1; i < len; i++) { // 1 to skip /[...]
    bool last = i == (len - 1);

    if (path[i] == '/' || last) {
      size_t length = i - lastslash - 1;
      if (last) // no need to remove trailing /
        length += 1;

      curr = ext2Traverse(ext2, curr, path + lastslash + 1, length);
      if (!curr)
        return curr;

      Ext2Inode *inode = ext2InodeFetch(ext2, curr);
      if ((inode->permission & 0xF000) == EXT2_S_IFLNK && (!last || follow)) {
        if (inode->size > 60) {
          debugf("[ext2::traverse::symlink] Todo! size{%d}\n", inode->size);
          free(inode);
          return 0;
        }
        char *start = (char *)inode->blocks;
        char *symlinkTarget =
            (char *)calloc(len + 60 + 2, 1); // extra just in case
        *symlinkResolve = symlinkTarget;

        if (start[0] != '/') {
          memcpy(symlinkTarget, path, lastslash + 1);
          memcpy(&symlinkTarget[lastslash + 1], start, inode->size);
          memcpy(&symlinkTarget[lastslash + 1 + inode->size],
                 &path[lastslash + 1 + length], len - (lastslash + 1 + length));
        } else {
          symlinkTarget[0] = '!';
          memcpy(&symlinkTarget[1], start, inode->size);
        }
        free(inode);
        return false;
      }
      free(inode);

      // return fail or last's success
      if (!curr || i == (len - 1))
        return curr;

      lastslash = i;
    }
  }

  // will never be reached but whatever
  return 0;
}
