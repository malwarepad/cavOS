#include <fat32.h>
#include <string.h>
#include <util.h>

size_t fat32ClusterToLBA(FAT32 *fat, uint32_t cluster) {
  return fat->offsetClusters + (cluster - 2) * fat->bootsec.sectors_per_cluster;
}

// todo: count forbidden chars (although not *really* needed)
#define IS_UPPERCASE(c) ((c) >= 'A' && (c) <= 'Z')
int fat32IsShortFilenamePossible(char *filename) {
  size_t len = strlength(filename);
  if (len > (8 + 1 + 3))
    return -1;

  int dotat = 0;
  for (int i = 0; i < len; i++) {
    if (filename[i] == '.')
      dotat = i;
    else if (!IS_UPPERCASE(filename[i]) && filename[i] != 0x20)
      return -1;
  }

  if (dotat && dotat < (len - 1 - 3))
    return -1;

  // you made it solider
  return dotat;
}

// Look, I know this looks junky and all, but what else do you have in mind?
// Seriously, why add any for() loops for 5 items or something, limiting the
// amount of optimizations the compiler can apply?
void fat32LFNmemcpy(uint8_t *lfnName, FAT32LFN *lfn, int index) {
  uint8_t *target = &lfnName[index * 13];
  target[0] = lfn->first_five[0];
  target[1] = lfn->first_five[2];
  target[2] = lfn->first_five[4];
  target[3] = lfn->first_five[6];
  target[4] = lfn->first_five[8];

  target[5] = lfn->next_six[0];
  target[6] = lfn->next_six[2];
  target[7] = lfn->next_six[4];
  target[8] = lfn->next_six[6];
  target[9] = lfn->next_six[8];
  target[10] = lfn->next_six[10];

  target[11] = lfn->last_two[0];
  target[12] = lfn->last_two[2];
}
