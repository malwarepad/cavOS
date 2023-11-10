#include <stdint.h>
#include <system.h>

#include "utils.h" // for itoa and atoi

void dumpHeapInfo() {
  int heap_start = syscallGetHeapStart();
  int heap_end = syscallGetHeapEnd();

  char heap_start_str[50];
  itoa(heap_start, heap_start_str);
  syscallTest(heap_start_str);

  char heap_end_str[50];
  itoa(heap_end, heap_end_str);
  syscallTest(heap_end_str);
}

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

  syscallTest("testing program heap...");
  dumpHeapInfo();

  syscallTest("adjusting program heap...");
  syscallAdjustHeapEnd(syscallGetHeapStart() + 0x1000);

  dumpHeapInfo();

  char *msg = "hello world!\n";
  for (int i = 0; i < strlength(msg); i++)
    syscallPrintChar(msg[i]);

  return 1;
}
