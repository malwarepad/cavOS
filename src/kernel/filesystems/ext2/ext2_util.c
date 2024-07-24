#include <ext2.h>
#include <malloc.h>
#include <system.h>
#include <util.h>

// *tmp has to be of blockSizeRounded
void ext2BlkIdBitmapFetch(Ext2 *ext2, uint8_t *tmp, size_t group) {
  // group is 0, because it's NOT relative, it's ABSOLUTE!
  getDiskBytes(tmp, BLOCK_TO_LBA(ext2, 0, ext2->bgdts[group].block_bitmap),
               ext2->blockSize / SECTOR_SIZE);
}

bool ext2BlkIdBitmapGet(Ext2 *ext2, uint8_t *tmp, size_t index) {
  size_t addr = index / 8;
  size_t offset = index % 8;

  bool ret = (tmp[addr] & (1 << offset)) != 0;
  return ret;
}

void ext2BlockFetchInit(Ext2 *ext2, Ext2LookupControl *control) {
  control->tmp1 = (uint32_t *)malloc(ext2->blockSize);
  control->tmp2 = (uint32_t *)malloc(ext2->blockSize);
}

void ext2BlockFetchCleanup(Ext2LookupControl *control) {
  if (control->tmp1)
    free(control->tmp1);
  if (control->tmp2)
    free(control->tmp2);
  control->tmp1Block = 0;
  control->tmp2Block = 0;
}

uint32_t ext2BlockFetch(Ext2 *ext2, Ext2Inode *ino, Ext2LookupControl *control,
                        size_t curr) {
  uint32_t itemsPerBlock = ext2->blockSize / sizeof(uint32_t);
  size_t   baseSingly = 12 + itemsPerBlock;
  size_t   baseDoubly = baseSingly + ext2->blockSize * itemsPerBlock;
  if (curr < 12)
    return ino->blocks[curr];
  else if (curr < baseSingly) {
    if (!ino->blocks[12])
      return 0;
    size_t tmp1block = BLOCK_TO_LBA(ext2, 0, ino->blocks[12]);
    if (control->tmp1Block != tmp1block) {
      control->tmp1Block = tmp1block;
      // control->tmp1 = (uint32_t *)malloc(ext2->blockSize);
      getDiskBytes((void *)control->tmp1, tmp1block,
                   ext2->blockSize / SECTOR_SIZE);
    }
    return control->tmp1[curr - 12];
  } else if (curr < baseDoubly) {
    if (!ino->blocks[13])
      return 0;
    size_t tmp1block = BLOCK_TO_LBA(ext2, 0, ino->blocks[13]);
    if (control->tmp1Block != tmp1block) {
      control->tmp1Block = tmp1block;
      // control->tmp2 = (uint32_t *)malloc(ext2->blockSize);
      getDiskBytes((void *)control->tmp1, tmp1block,
                   ext2->blockSize / SECTOR_SIZE);
    }

    size_t   at = curr - baseSingly;
    uint32_t index = at / itemsPerBlock;
    uint32_t rem = at % itemsPerBlock;
    size_t   tmp2block = BLOCK_TO_LBA(ext2, 0, control->tmp1[index]);
    if (!control->tmp1[index])
      return 0;
    if (control->tmp2Block != tmp2block) {
      control->tmp2Block = tmp2block;
      getDiskBytes((void *)control->tmp2, tmp2block,
                   ext2->blockSize / SECTOR_SIZE);
    }
    return control->tmp2[rem];
  }

  debugf("[ext2] TODO! Triply Indirect Block Pointer!\n");
  panic();
  return 0;
}

uint32_t *ext2BlockChain(Ext2 *ext2, Ext2OpenFd *fd, size_t curr,
                         size_t blocks) {
  uint32_t *ret = (uint32_t *)malloc((1 + blocks) * sizeof(uint32_t));
  for (int i = 0; i < (1 + blocks); i++) // will take care of curr too
  {
    ret[i] = ext2BlockFetch(ext2, &fd->inode, &fd->lookup, curr);
    curr++;
  }
  return ret;
}
