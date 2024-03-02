#include <fcntl.h>

extern void exit(int code);
int         main(int argc, char **argv);

void _start(int args) {
  int   *params = &args - 1;
  int    argc = *params;
  char **argv = (char **)(params + 1);

  // char** environ = argv + argc + 1;

  int ex = main(argc, argv);
  exit(ex);
}