#include <ext2.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

bool ext2Mount(MountPoint *mount) {
  // assign handlers
  mount->handlers = &ext2Handlers;
  mount->stat = ext2Stat;
  mount->lstat = ext2Lstat;

  // assign fsInfo
  mount->fsInfo = malloc(sizeof(Ext2));
  memset(mount->fsInfo, 0, sizeof(Ext2));
  Ext2 *ext2 = EXT2_PTR(mount->fsInfo);

  // base offset
  ext2->offsetBase = mount->mbr.lba_first_sector;
  ext2->offsetSuperblock = mount->mbr.lba_first_sector + 2;

  // get superblock
  uint8_t tmp[sizeof(Ext2Superblock)] = {0};
  getDiskBytes(tmp, ext2->offsetSuperblock, 2);

  // store it
  memcpy(&ext2->superblock, tmp, sizeof(Ext2Superblock));

  // checks
  if (ext2->superblock.ext2_magic != 0xEF53) {
    debugf("[ext2] Invalid magic number!\n");
    goto error;
  }

  if (ext2->superblock.major < 1) {
    debugf(
        "[ext2] FATAL! Ancient, pre-historic ext2 partition discovered! Please "
        "contact your local museum for further info...\n");
    goto error;
  }

  if (ext2->superblock.extended.required_feature != EXT2_R_F_TYPE_FIELD) {
    debugf("[ext2] FATAL! Unsupported flags detected: compression{%d} type{%d} "
           "replay{%d} device{%d}\n",
           ext2->superblock.extended.required_feature & EXT2_R_F_COMPRESSION,
           ext2->superblock.extended.required_feature & EXT2_R_F_TYPE_FIELD,
           ext2->superblock.extended.required_feature & EXT2_R_F_JOURNAL_REPLAY,
           ext2->superblock.extended.required_feature &
               EXT2_R_F_JOURNAL_DEVICE);
    goto error;
  }

  if (ext2->superblock.fs_state != EXT2_FS_S_CLEAN) {
    if (ext2->superblock.err == EXT2_FS_E_REMOUNT_RO) {
      debugf("[ext2] FATAL! Read-only partition!\n");
      goto error;
    } else if (ext2->superblock.err == EXT2_FS_E_KPANIC) {
      debugf("[ext2] FATAL! Superblock error caused panic!\n");
      panic();
    }
  }

  // log2.. why???!
  ext2->blockSize = 1024 << ext2->superblock.log2block_size;

  if ((ext2->blockSize % SECTOR_SIZE) != 0) {
    debugf("[ext2] FATAL! Block size is not sector-aligned! blockSize{%d}\n",
           ext2->blockSize);
    goto error;
  }

  // calculate block groups
  uint64_t blockGroups1 = DivRoundUp(ext2->superblock.total_blocks,
                                     ext2->superblock.blocks_per_group);
  uint64_t blockGroups2 = DivRoundUp(ext2->superblock.total_inodes,
                                     ext2->superblock.inodes_per_group);
  if (blockGroups1 != blockGroups2) {
    debugf("[ext2] Total block group calculation doesn't match up! 1{%ld} "
           "2{%ld}\n",
           blockGroups1, blockGroups2);
    goto error;
  }
  ext2->blockGroups = blockGroups1;

  // find the Block Group Descriptor Table
  // remember, very max is block size
  ext2->offsetBGDT = BLOCK_TO_LBA(ext2, 0, ext2->superblock.superblock_idx + 1);
  ext2->bgdts = (Ext2BlockGroup *)malloc(ext2->blockSize);
  getDiskBytes((void *)ext2->bgdts, ext2->offsetBGDT,
               DivRoundUp(ext2->blockSize, SECTOR_SIZE));

  // set up counting spinlocks for the BGDTs
  // remember to zero them, just in case
  int bgdtLockSize = sizeof(Spinlock) * ext2->blockGroups;
  ext2->LOCKS_BLOCK_BITMAP = (Spinlock *)malloc(bgdtLockSize);
  memset(ext2->LOCKS_BLOCK_BITMAP, 0, bgdtLockSize);

  ext2->inodeSize = ext2->superblock.extended.inode_size;
  ext2->inodeSizeRounded =
      DivRoundUp(ext2->inodeSize, SECTOR_SIZE) * SECTOR_SIZE;

  // done :")
  return true;

error:
  free(ext2);
  return false;
}

int ext2Open(char *filename, int flags, int mode, OpenFile *fd,
             char **symlinkResolve) {
  Ext2 *ext2 = EXT2_PTR(fd->mountPoint->fsInfo);

  uint32_t inode =
      ext2TraversePath(ext2, filename, EXT2_ROOT_INODE, true, symlinkResolve);

  if (!inode && *symlinkResolve)
    return -ENOENT;

  // todo! (mind symlink order later, this is just a hack)
  if (flags & O_CREAT || flags & O_WRONLY || flags & O_RDWR)
    return -EROFS;

  if (!inode)
    return -ENOENT;

  Ext2OpenFd *dir = (Ext2OpenFd *)malloc(sizeof(Ext2OpenFd));
  memset(dir, 0, sizeof(Ext2OpenFd));

  Ext2Inode *inodeFetched = ext2InodeFetch(ext2, inode);
  fd->dir = dir;

  dir->inodeNum = inode;
  memcpy(&dir->inode, inodeFetched, sizeof(Ext2Inode));

  if ((dir->inode.permission & 0xF000) == EXT2_S_IFDIR) {
    size_t len = strlength(filename) + 1;
    fd->dirname = malloc(len);
    memcpy(fd->dirname, filename, len);
  }

  ext2BlockFetchInit(ext2, &dir->lookup);

  // pointers & stuff
  dir->ptr = 0;

  free(inodeFetched);
  return 0;
}

int ext2Read(OpenFile *fd, uint8_t *buff, size_t naiveLimit) {
  Ext2       *ext2 = EXT2_PTR(fd->mountPoint->fsInfo);
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  size_t filesize = ext2GetFilesize(fd);
  if (dir->ptr >= filesize)
    return 0;

  size_t limit = naiveLimit;
  if (limit > (filesize - dir->ptr))
    limit = filesize - dir->ptr;

  size_t    blocksRequired = DivRoundUp(limit, ext2->blockSize);
  uint32_t *blocks =
      ext2BlockChain(ext2, dir, dir->ptr / ext2->blockSize, blocksRequired);

  uint8_t *tmp = (uint8_t *)malloc((blocksRequired + 1) * ext2->blockSize);
  int      currBlock = 0;

  // optimization: we can use consecutive sectors to make our life easier
  int consecStart = -1;
  int consecEnd = 0;

  // +1 for starting
  for (int i = 0; i < (blocksRequired + 1); i++) {
    if (!blocks[i])
      break;
    bool last = i == (blocksRequired - 1);
    if (consecStart < 0) {
      // nothing consecutive yet
      if (!last && blocks[i + 1] == (blocks[i] + 1)) {
        // consec starts here
        consecStart = i;
        continue;
      }
    } else {
      // we are in a consecutive that started since consecStart
      if (last || blocks[i + 1] != (blocks[i] + 1))
        consecEnd = i; // either last or the end
      else             // otherwise, we good
        continue;
    }

    if (consecEnd) {
      // optimized consecutive cluster reading
      int needed = consecEnd - consecStart + 1;
      getDiskBytes(&tmp[currBlock * ext2->blockSize],
                   BLOCK_TO_LBA(ext2, 0, blocks[consecStart]),
                   (needed * ext2->blockSize) / SECTOR_SIZE);
      currBlock += needed;
    } else {
      getDiskBytes(&tmp[currBlock * ext2->blockSize],
                   BLOCK_TO_LBA(ext2, 0, blocks[i]),
                   ext2->blockSize / SECTOR_SIZE);
      currBlock++;
    }

    // traverse
    consecStart = -1;
    consecEnd = 0;
  }

  // actually fill the buffer
  uint32_t offsetStarting = dir->ptr % ext2->blockSize; // remainder
  size_t   headtoCopy = MIN(ext2->blockSize - offsetStarting, limit);
  memcpy(buff, &tmp[offsetStarting], headtoCopy);
  if (limit > headtoCopy) {
    memcpy(&buff[headtoCopy], &tmp[ext2->blockSize], limit - headtoCopy);
  }

  dir->ptr += limit; // set pointer

  // cleanup
  free(blocks);
  free(tmp);

  // debugf("[fd:%d id:%d] read %d bytes\n", fd->id, currentTask->id, curr);
  // debugf("%d / %d\n", dir->ptr, dir->inode.size);
  return limit;
}

size_t ext2Seek(OpenFile *fd, size_t target, long int offset, int whence) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  // "hack" because openfile ptr is not used
  if (whence == SEEK_CURR)
    target += dir->ptr;

  if (target > ext2GetFilesize(fd))
    return -EINVAL;
  dir->ptr = target;

  return dir->ptr;
}

size_t ext2GetFilesize(OpenFile *fd) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);
  return COMBINE_64(dir->inode.size_high, dir->inode.size);
}

void ext2StatInternal(Ext2 *ext2, Ext2Inode *inode, uint32_t inodeNum,
                      struct stat *target) {
  target->st_dev = 69; // todo
  target->st_ino = inodeNum;
  // target->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR; // lie
  target->st_mode = inode->permission;
  target->st_nlink = inode->hard_links;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = ext2->blockSize;

  target->st_size = COMBINE_64(inode->size_high, inode->size);
  /*if ((inode->permission & 0xF000) == 0x4000) { // lies
    target->st_mode &= ~S_IFREG;                // mark as dir
    target->st_mode |= S_IFDIR;
  } else if ((inode->permission & 0xF000) == 0xA000) {
    target->st_mode &= ~S_IFREG; // mark as symlink
    target->st_mode |= S_IFLNK;
  }*/

  target->st_blocks =
      (DivRoundUp(target->st_size, target->st_blksize) * target->st_blksize) /
      512;

  target->st_atime = inode->atime;
  target->st_mtime = inode->mtime;
  target->st_ctime = inode->ctime;
}

bool ext2Stat(MountPoint *mnt, char *filename, struct stat *target,
              char **symlinkResolve) {
  Ext2    *ext2 = EXT2_PTR(mnt->fsInfo);
  uint32_t inodeNum =
      ext2TraversePath(ext2, filename, EXT2_ROOT_INODE, true, symlinkResolve);
  if (!inodeNum)
    return false;
  Ext2Inode *inode = ext2InodeFetch(ext2, inodeNum);

  ext2StatInternal(ext2, inode, inodeNum, target);

  free(inode);
  return true;
}

bool ext2Lstat(MountPoint *mnt, char *filename, struct stat *target,
               char **symlinkResolve) {
  Ext2    *ext2 = EXT2_PTR(mnt->fsInfo);
  uint32_t inodeNum =
      ext2TraversePath(ext2, filename, EXT2_ROOT_INODE, false, symlinkResolve);
  if (!inodeNum)
    return false;
  Ext2Inode *inode = ext2InodeFetch(ext2, inodeNum);

  ext2StatInternal(ext2, inode, inodeNum, target);

  free(inode);
  return true;
}

int ext2StatFd(OpenFile *fd, struct stat *target) {
  Ext2       *ext2 = EXT2_PTR(fd->mountPoint->fsInfo);
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);
  ext2StatInternal(ext2, &dir->inode, dir->inodeNum, target);
  return 0;
}

int ext2Readlink(Ext2 *ext2, char *path, char *buf, int size,
                 char **symlinkResolve) {
  if (size < 0)
    return -EINVAL;
  else if (!size)
    return 0;

  uint32_t inodeNum =
      ext2TraversePath(ext2, path, EXT2_ROOT_INODE, false, symlinkResolve);
  if (!inodeNum)
    return -ENOENT;

  int ret = -1;

  Ext2Inode *inode = ext2InodeFetch(ext2, inodeNum);
  if ((inode->permission & 0xF000) != 0xA000) {
    ret = -EINVAL;
    goto cleanup;
  }

  if (inode->size > 60) {
    debugf("[ext2::symlink] Todo! size{%d}\n", inode->size);
    ret = -1;
    goto cleanup;
  }

  char *start = (char *)inode->blocks;
  int   toCopy = inode->size;
  if (toCopy > size)
    toCopy = size;

  memcpy(buf, start, toCopy);
  ret = toCopy;

cleanup:
  free(inode);
  return ret;
}

bool ext2Close(OpenFile *fd) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  ext2BlockFetchCleanup(&dir->lookup);

  free(fd->dir);
  return true;
}

bool ext2DuplicateNodeUnsafe(OpenFile *original, OpenFile *orphan) {
  orphan->dir = malloc(sizeof(Ext2OpenFd));
  memcpy(orphan->dir, original->dir, sizeof(Ext2OpenFd));

  Ext2       *ext2 = EXT2_PTR(orphan->mountPoint->fsInfo);
  Ext2OpenFd *dir = EXT2_DIR_PTR(orphan->dir);
  Ext2OpenFd *dirOriginal = EXT2_DIR_PTR(original->dir);

  if (dir->lookup.tmp1) {
    dir->lookup.tmp1 = malloc(ext2->blockSize);
    memcpy(dir->lookup.tmp1, dirOriginal->lookup.tmp1, ext2->blockSize);
  }

  if (dir->lookup.tmp2) {
    dir->lookup.tmp2 = malloc(ext2->blockSize);
    memcpy(dir->lookup.tmp2, dirOriginal->lookup.tmp2, ext2->blockSize);
  }

  if (original->dirname) {
    size_t len = strlength(original->dirname) + 1;
    orphan->dirname = (char *)malloc(len);
    memcpy(orphan->dirname, original->dirname, len);
  }

  return true;
}

VfsHandlers ext2Handlers = {.open = ext2Open,
                            .close = ext2Close,
                            .duplicate = ext2DuplicateNodeUnsafe,
                            .read = ext2Read,
                            .stat = ext2StatFd,
                            .getdents64 = ext2Getdents64,
                            .seek = ext2Seek,
                            .getFilesize = ext2GetFilesize};
