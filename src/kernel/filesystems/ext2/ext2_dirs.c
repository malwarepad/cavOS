#include <dents.h>
#include <ext2.h>
#include <malloc.h>
#include <system.h>
#include <timer.h>
#include <util.h>

bool ext2DirAllocate(Ext2 *ext2, uint32_t inodeNum, Ext2Inode *parentDirInode,
                     char *filename, uint8_t filenameLen, uint8_t type,
                     uint32_t inode) {
  int actuallyHoldingLock =
      ext2DirLock(ext2->dirOperations, &ext2->LOCK_DIRALLOC_GLOBAL,
                  EXT2_MAX_CONSEC_DIRALLOC, inodeNum);
  int entryLen = sizeof(Ext2Directory) + filenameLen;

  Ext2Inode *ino = parentDirInode; // <- todo
  uint8_t   *names = (uint8_t *)malloc(ext2->blockSize);

  Ext2LookupControl control = {0};
  size_t            blockNum = 0;

  bool ret = false;
  ext2BlockFetchInit(ext2, &control);

  int blocksContained = DivRoundUp(ino->size, ext2->blockSize);
  for (int i = 0; i < blocksContained; i++) {
    size_t block = ext2BlockFetch(ext2, ino, &control, blockNum);
    if (!block)
      break;
    blockNum++;
    Ext2Directory *dir = (Ext2Directory *)names;

    getDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                 ext2->blockSize / SECTOR_SIZE);

    while (((size_t)dir - (size_t)names) < ext2->blockSize) {
      if (dir->inode && filenameLen == dir->filenameLength &&
          memcmp(dir->filename, filename, filenameLen) == 0) {
        ret = false;
        goto cleanup;
      }
      int minForOld = (sizeof(Ext2Directory) + dir->filenameLength + 3) & ~3;
      int minForNew = (entryLen + 3) & ~3;

      int remainderForNew = dir->size - minForOld;
      if (remainderForNew < minForNew)
        goto next_item;

      // means we now have enough space to put the new one in

      dir->size = minForOld;
      Ext2Directory *new = (void *)((size_t)dir + dir->size);
      new->size = remainderForNew;

      new->type = type;
      memcpy(new->filename, filename, filenameLen);
      new->filenameLength = filenameLen;
      new->inode = inode;

      setDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                   ext2->blockSize / SECTOR_SIZE);

      ret = true;
      goto cleanup;

    next_item:
      dir = (void *)((size_t)dir + dir->size);
    }
  }

  // means we need to allocate another block for these
  uint32_t group = INODE_TO_BLOCK_GROUP(ext2, inodeNum);
  uint32_t newBlock = ext2BlockFind(ext2, group, 1);

  uint8_t *newBlockBuff = names; // reuse names :p
  getDiskBytes(newBlockBuff, BLOCK_TO_LBA(ext2, 0, newBlock),
               ext2->blockSize / SECTOR_SIZE);

  Ext2Directory *new = (Ext2Directory *)(newBlockBuff);
  new->size = ext2->blockSize;
  new->type = type;
  memcpy(new->filename, filename, filenameLen);
  new->filenameLength = filenameLen;
  new->inode = inode;

  setDiskBytes(newBlockBuff, BLOCK_TO_LBA(ext2, 0, newBlock),
               ext2->blockSize / SECTOR_SIZE);
  ext2BlockAssign(ext2, ino, inodeNum, &control, blockNum, newBlock);

  int lockAt = ext2DirLock(ext2->inodeOperations, &ext2->LOCK_BLOCK_INODE,
                           EXT2_MAX_CONSEC_INODE, inode);
  ino->num_sectors += ext2->blockSize / SECTOR_SIZE;
  ino->size += ext2->blockSize;
  ext2InodeModifyM(ext2, inodeNum, ino);
  ext2->inodeOperations[lockAt] = 0;

  ret = true;

cleanup:
  ext2BlockFetchCleanup(&control);
  free(names);
  ext2->dirOperations[actuallyHoldingLock] = 0;

  return ret;
}

int ext2Getdents64(OpenFile *file, struct linux_dirent64 *start,
                   unsigned int hardlimit) {
  Ext2       *ext2 = EXT2_PTR(file->mountPoint->fsInfo);
  Ext2OpenFd *edir = EXT2_DIR_PTR(file->dir);

  if ((edir->inode.permission & 0xF000) != EXT2_S_IFDIR)
    return -ENOTDIR;

  int        allocatedlimit = 0;
  Ext2Inode *ino = &edir->inode;
  uint8_t   *names = (uint8_t *)malloc(ext2->blockSize);

  struct linux_dirent64 *dirp = (struct linux_dirent64 *)start;

  int blocksContained = DivRoundUp(ino->size, ext2->blockSize);
  for (int i = 0; i < blocksContained; i++) {
    size_t block =
        ext2BlockFetch(ext2, ino, &edir->lookup, edir->ptr / ext2->blockSize);
    if (!block)
      break;
    Ext2Directory *dir =
        (Ext2Directory *)((size_t)names + (edir->ptr % ext2->blockSize));

    getDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                 ext2->blockSize / SECTOR_SIZE);

    while (((size_t)dir - (size_t)names) < ext2->blockSize) {
      if (!dir->inode)
        continue;

      unsigned char type = 0;
      if (dir->type == 2)
        type = CDT_DIR;
      else if (dir->type == 7)
        type = CDT_LNK;
      else
        type = CDT_REG;

      DENTS_RES res =
          dentsAdd(start, &dirp, &allocatedlimit, hardlimit, dir->filename,
                   dir->filenameLength, dir->inode, type);

      if (res == DENTS_NO_SPACE) {
        allocatedlimit = -EINVAL;
        goto cleanup;
      } else if (res == DENTS_RETURN)
        goto cleanup;

      edir->ptr += dir->size;
      dir = (void *)((size_t)dir + dir->size);
    }

    int rem = edir->ptr % ext2->blockSize;
    if (rem)
      edir->ptr += ext2->blockSize - rem;
  }

cleanup:
  free(names);
  return allocatedlimit;
}