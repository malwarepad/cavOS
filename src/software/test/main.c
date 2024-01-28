#include <stdint.h>
#include <stdio.h>
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

FILE *openControlled(char *filename) {
  FILE *res = fopen(filename, "r");
  printf("Opened file res{%d} filename{%s}\n", res == 0, filename);
  return res;
}

int closeControlled(FILE *file) {
  int res = fclose(file);
  printf("Closed file res{%d}\n", res);
  return res;
}

int main(int argc, char **argv) {
  argc = argc;
  argv = argv;

  printf("Process PID: %ld\n", syscallGetPid());

  int dec;
  scanf("%d", &dec);
  printf("\nyou entered: %d\n", dec);

  // FILE *file1 = openControlled("/files/lorem.txt");
  // FILE *file2 = openControlled("/files/untitled.txt");
  // FILE *file3 = openControlled("/files/ab.txt");
  // FILE *file4 = openControlled("/software/testing.cav");

  // closeControlled(file1);
  // closeControlled(file2);
  // closeControlled(file3);
  // closeControlled(file4);

  for (int i = 0; i < argc; i++) {
    syscallTest(argv[i]);
  }

  syscallTest("testing program heap...");
  dumpHeapInfo();

  syscallTest("adjusting program heap...");
  syscallAdjustHeapEnd(syscallGetHeapStart() + 0x1000);

  dumpHeapInfo();

  char *msg = "hello world!\n";
  syscallWrite(0, msg, strlength(msg));

  return 1;
}
