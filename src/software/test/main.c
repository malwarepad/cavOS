#include <stdint.h>
#include <system.h>

#include "utils.h" // for itoa and atoi

int main(int argc, char **argv) {
  argc = argc;
  argv = argv;

  int  pid = syscallGetPid();
  char str[50];
  itoa(pid, str);

  syscallTest(str);

  for (int i = 0; i < argc; i++) {
    syscallTest(argv[i]);
  }

  return 1;
}
