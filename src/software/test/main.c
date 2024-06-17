#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  pid_t pid;
  int   status;

  // Fork a new process
  pid = fork();

  if (pid < 0) {
    // Fork failed
    perror("Fork failed");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    // Child process
    char *argv[] = {"/usr/bin/bash", "-c", "/asdfiygfasdiugfsa7tyfs", NULL};
    char *envp[] = {NULL};

    if (execve(argv[0], argv, envp) == -1) {
      perror("execve failed");
      exit(EXIT_FAILURE);
    }
  } else {
    // Parent process
    struct rusage usage;

    // Wait for the child process to finish and get resource usage
    if (wait4(-1, &status, 0, &usage) == -1) {
      perror("wait4 failed");
      exit(EXIT_FAILURE);
    }

    // Check if the child process terminated normally
    if (WIFEXITED(status)) {
      printf("Child process exited with status %d\n", WEXITSTATUS(status));
    } else {
      printf("Child process did not terminate normally\n");
    }

    printf("ffff %lx\n", status);

    // Print some resource usage statistics
    printf("User CPU time used: %ld.%06ld seconds\n", usage.ru_utime.tv_sec,
           usage.ru_utime.tv_usec);
    printf("System CPU time used: %ld.%06ld seconds\n", usage.ru_stime.tv_sec,
           usage.ru_stime.tv_usec);
  }

  return 0;
}
