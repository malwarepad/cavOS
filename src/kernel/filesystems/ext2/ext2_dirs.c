#include <dents.h>
#include <ext2.h>
#include <malloc.h>
#include <system.h>
#include <timer.h>
#include <util.h>

int ext2Getdents64(OpenFile *file, void *start, unsigned int hardlimit) {
  Ext2       *ext2 = EXT2_PTR(file->mountPoint->fsInfo);
  Ext2OpenFd *edir = EXT2_DIR_PTR(file->dir);

  if ((edir->inode.permission & 0xF000) != EXT2_S_IFDIR)
    return -ENOTDIR;

  int        allocatedlimit = 0;
  Ext2Inode *ino = &edir->inode;
  uint8_t   *names = (uint8_t *)malloc(ext2->blockSize);

  struct linux_dirent64 *dirp = (struct linux_dirent64 *)start;

  int dirsAvailable = 0;
  while (true) {
    size_t block =
        ext2BlockFetch(ext2, ino, &edir->lookup, edir->ptr / ext2->blockSize);
    if (!block)
      break;
    Ext2Directory *dir =
        (Ext2Directory *)((size_t)names + (edir->ptr % ext2->blockSize));

    getDiskBytes(names, BLOCK_TO_LBA(ext2, 0, block),
                 ext2->blockSize / SECTOR_SIZE);

    while (((size_t)dir - (size_t)names) < ext2->blockSize) {
      if (++dirsAvailable >= COMBINE_64(ino->size_high, ino->size))
        break;

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