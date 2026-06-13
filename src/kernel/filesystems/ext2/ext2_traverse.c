#include <ext2.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <timer.h>
#include <util.h>

#include <linked_list.h>
#include <murmur_hash.h>

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

// bool ext2TraverseCb(void *data, void *ctx) {
//   size_t           inode = (size_t)ctx;
//   Ext2FoundObject *obj = data;
//   return obj->inode == inode;
// }

typedef struct Ext2TrCbStr {
  char  *filename;
  size_t len;

  uint64_t hash;
} Ext2TrCbStr;
bool ext2TraverseCbStr(void *data, void *ctx) {
  Ext2TrCbStr     *target = ctx;
  Ext2FoundObject *obj = data;
  if (target->len != obj->filenameLen)
    return false;
  // use hashing only for rejecting, dont depend on it
  if (target->hash != obj->hash)
    return false;
  return memcmp(obj->filename, target->filename, target->len) == 0;
}

uint32_t ext2TraversePath(Ext2 *ext2, char *path, Ext2FoundObject **retObj,
                          bool follow, char **symlinkResolve) {
  uint32_t curr = EXT2_ROOT_INODE;
  size_t   len = strlength(path);

  if (len == 1) { // meaning it's trying to open /
    if (retObj)
      *retObj = (Ext2FoundObject *)ext2->rootObject.firstObject;
    return 2;
  }

  uint32_t ret = 0;

  spinlockAcquire(&ext2->LOCK_OBJECT);
  Ext2FoundObject *obj = (Ext2FoundObject *)ext2->rootObject.firstObject;

  Ext2TrCbStr cb = {0};

  int lastslash = 0;
  for (int i = 1; i < len; i++) { // 1 to skip /[...]
    bool last = i == (len - 1);

    if (path[i] == '/' || last) {
      size_t length = i - lastslash - 1;
      if (last) // no need to remove trailing /
        length += 1;

      cb.filename = path + lastslash + 1;
      cb.len = length;
      cb.hash = murmur_hash(cb.filename, cb.len, 67);
      Ext2FoundObject *objNext =
          LinkedListSearch(&obj->inner, ext2TraverseCbStr, &cb);

      if (objNext) {
        obj = objNext;
        curr = objNext->inode;
      } else
        curr = ext2Traverse(ext2, curr, path + lastslash + 1, length);

      if (!curr) {
        ret = curr;
        goto cleanup;
      }

      if (!objNext) {
        // create it for later traverses
        Ext2FoundObject *new =
            LinkedListAllocate(&obj->inner, sizeof(Ext2FoundObject));
        LinkedListInit(&new->inner, sizeof(Ext2FoundObject));
        new->inode = curr;
        new->filename = malloc(cb.len + 1);
        memcpy(new->filename, cb.filename, cb.len);
        new->filename[cb.len] = '\0';
        new->filenameLen = cb.len;
        new->hash = cb.hash;
        objNext = new;
        obj = objNext;
      }

      Ext2Inode *inode = ext2InodeFetch(ext2, curr);
      if ((inode->permission & 0xF000) == EXT2_S_IFLNK && (!last || follow)) {
        char *start = 0;
        char *symlinkTarget = 0;
        if (inode->size > 60) {
          assert(inode->size < ext2->blockSize);

          start = (char *)calloc(ext2->blockSize + 1, 1);
          symlinkTarget = (char *)calloc(len + inode->size + 2,
                                         1); // extra just in case
          getDiskBytes((uint8_t *)start,
                       BLOCK_TO_LBA(ext2, 0, inode->blocks[0]),
                       ext2->blockSize / SECTOR_SIZE);
        } else {
          start = (char *)inode->blocks;
          symlinkTarget = (char *)calloc(len + 60 + 2, 1); // extra just in case
        }
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
        if (inode->size > 60)
          free(start);
        free(inode);

        ret = 0;
        goto cleanup;
      }
      bool notdir = !(inode->permission & S_IFDIR);
      free(inode);

      // return fail or last's success
      if (!curr || i == (len - 1)) {
        ret = curr;
        goto cleanup;
      }

      if (notdir) {
        // if by this point it's not a directory, we're screwed
        ret = 0;
        goto cleanup;
      }

      lastslash = i;
    }
  }

cleanup:
  if (ret >= 2 && retObj) {
    atomic_fetch_add(&obj->openFds, 1);
    *retObj = obj;
  }
  spinlockRelease(&ext2->LOCK_OBJECT);
  return ret;
}
