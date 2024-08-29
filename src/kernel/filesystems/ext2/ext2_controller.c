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

  ext2->inodeSize = ext2->superblock.extended.inode_size;
  ext2->inodeSizeRounded =
      DivRoundUp(ext2->inodeSize, SECTOR_SIZE) * SECTOR_SIZE;

  // done :")
  return true;

error:
  free(ext2);
  return false;
}

bool ext2Open(MountPoint *mount, OpenFile *fd, char *filename,
              char **symlinkResolve) {
  Ext2 *ext2 = EXT2_PTR(mount->fsInfo);

  uint32_t inode =
      ext2TraversePath(ext2, filename, EXT2_ROOT_INODE, true, symlinkResolve);
  if (!inode)
    return false;

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
  return true;
}

int ext2Read(MountPoint *mount, OpenFile *fd, uint8_t *buff, int limit) {
  Ext2       *ext2 = EXT2_PTR(mount->fsInfo);
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  size_t filesize = ext2GetFilesize(fd);
  if (dir->ptr >= filesize)
    return 0;

  size_t    blocksRequired = DivRoundUp(limit, ext2->blockSize);
  uint32_t *blocks =
      ext2BlockChain(ext2, dir, dir->ptr / ext2->blockSize, blocksRequired);
  uint8_t *bytes = (uint8_t *)malloc(ext2->blockSize);

  int curr = 0; // will be used to return

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

    uint32_t offsetStarting = dir->ptr % ext2->blockSize; // remainder
    if (consecEnd) {
      // optimized consecutive cluster reading
      int      needed = consecEnd - consecStart + 1;
      uint8_t *optimizedBytes = malloc(needed * ext2->blockSize);
      getDiskBytes(optimizedBytes, BLOCK_TO_LBA(ext2, 0, blocks[consecStart]),
                   (needed * ext2->blockSize) / SECTOR_SIZE);

      for (uint32_t i = offsetStarting; i < (needed * ext2->blockSize); i++) {
        if (curr >= limit || dir->ptr >= filesize) {
          free(optimizedBytes);
          goto cleanup;
        }

        if (buff)
          buff[curr] = optimizedBytes[i];

        dir->ptr++;
        curr++;
      }

      free(optimizedBytes);
    } else {
      getDiskBytes(bytes, BLOCK_TO_LBA(ext2, 0, blocks[i]),
                   ext2->blockSize / SECTOR_SIZE);

      for (uint32_t i = offsetStarting; i < ext2->blockSize; i++) {
        if (curr >= limit || dir->ptr >= filesize)
          goto cleanup;

        if (buff)
          buff[curr] = bytes[i];

        dir->ptr++;
        curr++;
      }
    }

    // traverse
    consecStart = -1;
    consecEnd = 0;
  }

cleanup:
  free(bytes);
  free(blocks);
  // debugf("[fd:%d id:%d] read %d bytes\n", fd->id, currentTask->id, curr);
  // debugf("%d / %d\n", dir->ptr, dir->inode.size);
  return curr;
}

bool ext2Seek(MountPoint *mount, OpenFile *fd, uint32_t target) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  if (target > ext2GetFilesize(fd))
    return false;
  dir->ptr = target;
  return true;
}

size_t ext2GetFilesize(OpenFile *fd) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);
  return COMBINE_64(dir->inode.size_high, dir->inode.size);
}

void ext2StatInternal(Ext2Inode *inode, uint32_t inodeNum,
                      struct stat *target) {
  target->st_dev = 69; // todo
  target->st_ino = inodeNum;
  target->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR;
  target->st_nlink = inode->hard_links;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = 0x1000; // todo: honesty :")

  target->st_size = COMBINE_64(inode->size_high, inode->size);
  if ((inode->permission & 0xF000) == 0x4000) {
    target->st_size = 0x1000;

    target->st_mode &= ~S_IFREG; // mark as dir
    target->st_mode |= S_IFDIR;
  } else if ((inode->permission & 0xF000) == 0xA000) {
    target->st_mode &= ~S_IFREG; // mark as symlink
    target->st_mode |= S_IFLNK;
  }

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

  ext2StatInternal(inode, inodeNum, target);

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

  ext2StatInternal(inode, inodeNum, target);

  free(inode);
  return true;
}

bool ext2StatFd(Ext2 *ext2, OpenFile *fd, struct stat *target) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);
  ext2StatInternal(&dir->inode, dir->inodeNum, target);
  return true;
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

bool ext2Close(MountPoint *mount, OpenFile *fd) {
  Ext2OpenFd *dir = EXT2_DIR_PTR(fd->dir);

  ext2BlockFetchCleanup(&dir->lookup);

  free(fd->dir);
  return true;
}

// todo!
VfsHandlers ext2Handlers = {.open = fsSpecificOpen,
                            .close = fsSpecificClose,
                            .duplicate = fsSpecificDuplicateNodeUnsafe,
                            .ioctl = fsSpecificIoctl,
                            .mmap = 0,
                            .read = fsSpecificRead,
                            .stat = fsSpecificStat,
                            .write = fsSpecificWrite,
                            .getdents64 = fsSpecificGetdents64,
                            .seek = fsSpecificSeek};
