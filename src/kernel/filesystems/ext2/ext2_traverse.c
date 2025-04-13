#include <ext2.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <timer.h>
#include <util.h>

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
    size_t block = ext2BlockFetch(ext2, ino, initInode, &control, blockNum);
    blockNum++;
    if (!block)
      break;
    Ext2Directory *dir = (Ext2Directory *)names;

    getDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                 ext2->blockSize / SECTOR_SIZE);

    while (((size_t)dir - (size_t)names) < ext2->blockSize) {
      if (!dir->inode) {
        goto traverse;
        continue;
      }
      if (dir->filenameLength == searchLength &&
          memcmp(dir->filename, search, searchLength) == 0) {
        ret = dir->inode;
        goto cleanup;
      }
    traverse:
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
      bool notdir = !(inode->permission & S_IFDIR);
      free(inode);

      // return fail or last's success
      if (!curr || i == (len - 1))
        return curr;

      if (notdir) {
        // if by this point it's not a directory, we're screwed
        return false;
      }

      lastslash = i;
    }
  }

  // will never be reached but whatever
  return 0;
}
