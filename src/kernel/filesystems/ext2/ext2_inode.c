#include <ext2.h>
#include <malloc.h>
#include <system.h>
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

uint32_t ext2InodeFind(Ext2 *ext2, int groupSuggestion) {
  if (ext2->superblock.free_inodes < 1)
    goto burn;

  uint32_t suggested = ext2InodeFindL(ext2, groupSuggestion);
  if (suggested)
    return suggested;

  for (int i = 0; i < ext2->blockGroups; i++) {
    if (i == groupSuggestion)
      continue;

    uint32_t ret = ext2InodeFindL(ext2, i);
    if (ret)
      return ret;
  }

burn:
  debugf("[ext2] FATAL! Couldn't find a single inode! Drive is full!\n");
  panic();
  return 0;
}

uint32_t ext2InodeFindL(Ext2 *ext2, int group) {
  if (ext2->bgdts[group].free_inodes < 1)
    return 0;

  spinlockAcquire(&ext2->LOCKS_INODE_BITMAP[group]);

  uint32_t ret = 0;
  uint8_t *buff = malloc(ext2->blockSize);

  getDiskBytes(buff, BLOCK_TO_LBA(ext2, 0, ext2->bgdts[group].inode_bitmap),
               ext2->blockSize / SECTOR_SIZE);

  int firstInodeDiv = 0;
  int firstInodeRem = 0;

  if (group == 0) {
    firstInodeDiv = ext2->superblock.extended.first_inode / 8;
    firstInodeRem = ext2->superblock.extended.first_inode % 8;
  }

  for (int i = firstInodeDiv; i < ext2->blockSize; i++) {
    if (buff[i] == 0xff)
      continue;
    for (int j = (i == firstInodeDiv ? firstInodeRem : 0); j < 8; j++) {
      if (!(buff[i] & (1 << j))) {
        ret = i * 8 + j;
        goto cleanup;
      }
    }
  }

cleanup:
  if (ret) {
    // we found blocks successfully, mark them as allocated
    uint32_t where = ret / 8;
    uint32_t remainder = ret % 8;
    buff[where] |= (1 << remainder);
    setDiskBytes(buff, BLOCK_TO_LBA(ext2, 0, ext2->bgdts[group].inode_bitmap),
                 ext2->blockSize / SECTOR_SIZE);

    // set the bgdt accordingly
    spinlockAcquire(&ext2->LOCK_BGDT_WRITE);
    ext2->bgdts[group].free_inodes--;
    ext2BgdtPushM(ext2);
    spinlockRelease(&ext2->LOCK_BGDT_WRITE);

    // and the superblock
    spinlockAcquire(&ext2->LOCK_SUPERBLOCK_WRITE);
    ext2->superblock.free_inodes--;
    ext2SuperblockPushM(ext2);
    spinlockRelease(&ext2->LOCK_SUPERBLOCK_WRITE);
  }

  spinlockRelease(&ext2->LOCKS_INODE_BITMAP[group]);
  return ret ? (group * ext2->superblock.inodes_per_group + ret) : 0;
}
