extern int main(int argc, char *argv[]);

void _start() {
  int   argc = syscallGetArgc();
  char *argv[argc];

  for (int i = 0; i < argc; i++) {
    argv[i] = syscallGetArgv(i);
  }

  int returnCode = main(argc, argv);
  syscallExitTask(returnCode);
}