#include <fat32.h>
#include <rtc.h>
#include <string.h>
#include <util.h>

size_t fat32ClusterToLBA(FAT32 *fat, uint32_t cluster) {
  return fat->offsetClusters + (cluster - 2) * fat->bootsec.sectors_per_cluster;
}

// todo: count forbidden chars (although not *really* needed)
#define IS_UPPERCASE(c)                                                        \
  (((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9'))
int fat32IsShortFilenamePossible(char *filename, size_t len) {
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

int fat32SFNtoNormal(uint8_t *target, FAT32DirectoryEntry *dirent) {
  int i;
  for (i = 0; i < 8 && dirent->name[i] != ' '; i++)
    target[i] = dirent->name[i];

  // add a dot if there is an extension
  if (dirent->ext[0] != ' ' && dirent->ext[0] != '\0') {
    target[i++] = '.';

    // copy the extension part, ensuring no trailing spaces are copied
    for (int j = 0; j < 3 && dirent->ext[j] != ' '; ++j) {
      target[i++] = dirent->ext[j];
    }
  }

  target[i] = '\0'; // null terminator
  return i + 1;
}

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR (60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY (24 * SECONDS_PER_HOUR)
#define SECONDS_FROM_1970_TO_1980                                              \
  (315532800) // Number of seconds from 1970 to 1980

// Get the number of days in a given month of a given year
int days_in_month(int year, int month) {
  static const int days_per_month[] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) {
    return 29;
  } else {
    return days_per_month[month - 1];
  }
}

// Calculate the number of days from 1980-01-01 to the given date
int days_since_1980(int year, int month, int day) {
  int days = 0;
  for (int y = 1980; y < year; y++) {
    days += isLeapYear(y) ? 366 : 365;
  }
  for (int m = 1; m < month; m++) {
    days += days_in_month(year, m);
  }
  days += day - 1;
  return days;
}

// Convert FAT date and time to Unix time
unsigned long fat32UnixTime(unsigned short fat_date, unsigned short fat_time) {
  int year = ((fat_date >> 9) & 0x7F) + 1980;
  int month = (fat_date >> 5) & 0x0F;
  int day = fat_date & 0x1F;

  int hour = (fat_time >> 11) & 0x1F;
  int minute = (fat_time >> 5) & 0x3F;
  int second = (fat_time & 0x1F) * 2;

  // Calculate total days since 1970-01-01
  int days = days_since_1980(year, month, day) +
             (365 * 10 + 2); // Days from 1970 to 1980
  unsigned long total_seconds = days * SECONDS_PER_DAY +
                                hour * SECONDS_PER_HOUR +
                                minute * SECONDS_PER_MINUTE + second;

  return total_seconds;
}
