#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "utils.h" // for itoa and atoi

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
  printf("args: %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("arg%d: %s\n", i, argv[i]);
  }

  // int dec;
  // scanf("%d", &dec);
  // printf("\nyou entered: %d\n", dec);

  // FILE *file1 = openControlled("/files/lorem.txt");
  // FILE *file2 = openControlled("/files/untitled.txt");
  // FILE *file3 = openControlled("/files/ab.txt");
  // FILE *file4 = openControlled("/software/testing.cav");

  // closeControlled(file1);
  // closeControlled(file2);
  // closeControlled(file3);
  // closeControlled(file4);

  // for (int i = 0; i < argc; i++) {
  //   syscallTest(argv[i]);
  // }

  char *msg = "hello world!\n";
  write(0, msg, strlength(msg));

  return 1;
}
