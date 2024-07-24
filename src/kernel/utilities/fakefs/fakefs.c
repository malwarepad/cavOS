#include <fakefs.h>
#include <task.h>

void initiateFakefs() {
  fsUserOpenSpecial((void **)(&firstGlobalSpecial), "/dev/null", currentTask,
                    -1, &handleNull);
}
