#include <fcntl.h>

extern void exit(int code);
int         main(int argc, char **argv);

void _start(int argc, char **argv) {
  int ex = main(argc, argv);
  exit(ex);
}