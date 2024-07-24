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

  int64_t    allocatedlimit = 0;
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
      size_t reclen = 23 + dir->filenameLength + 1;
      if ((allocatedlimit + reclen + 2) > hardlimit)
        goto cleanup; // todo: error code special
      dirp->d_ino = dir->inode;
      dirp->d_off = rand(); // xd
      if (dir->type == 2)
        dirp->d_type = CDT_DIR;
      else if (dir->type == 7)
        dirp->d_type = CDT_LNK;
      else
        dirp->d_type = CDT_REG;
      memcpy(dirp->d_name, dir->filename, dir->filenameLength);
      dirp->d_name[dir->filenameLength] = '\0';
      dirp->d_reclen = reclen;

      allocatedlimit += reclen;
      dirp = (struct linux_dirent64 *)((size_t)dirp + dirp->d_reclen);
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