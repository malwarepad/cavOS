#include <stdint.h>

#define SYSCALL_TEST 0x0
#define SYSCALL_EXIT_TASK 0x1
#define SYSCALL_GETPID 0x2
#define SYSCALL_GETARGC 0x3
#define SYSCALL_GETARGV 0x4

void     syscallTest(char *msg);
void     syscallExitTask(int return_code);
uint32_t syscallGetPid();
int      syscallGetArgc();
char    *syscallGetArgv(int curr);
