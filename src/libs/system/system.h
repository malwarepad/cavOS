#include <stdint.h>

#define SYSCALL_TEST 0x0
#define SYSCALL_EXIT_TASK 0x1
#define SYSCALL_GETPID 0x2
#define SYSCALL_GETARGC 0x3
#define SYSCALL_GETARGV 0x4
#define SYSCALL_GET_HEAP_START 0x5
#define SYSCALL_GET_HEAP_END 0x6
#define SYSCALL_ADJUST_HEAP_END 0x7
#define SYSCALL_PRINT_CHAR 0x8

void     syscallTest(char *msg);
void     syscallExitTask(int return_code);
uint32_t syscallGetPid();
int      syscallGetArgc();
char    *syscallGetArgv(int curr);
uint32_t syscallGetHeapStart();
uint32_t syscallGetHeapEnd();
void     syscallAdjustHeapEnd(uint32_t heap_end);
void     syscallPrint(char *str, uint32_t count);
