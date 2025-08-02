#include "isr.h"
#include "system.h"
#include "types.h"
#include "vfs.h"

#ifndef TASK_H
#define TASK_H

// "Descriptor Privilege Level"
#define DPL_USER 3

#define KERNEL_TASK_ID 0

#define entryCmdline ("kernel")
#define helperCmdline ("kernel")
#define dummyCmdline ("dummy")
#define lwipCmdline ("lwip")

typedef struct {
  uint64_t edi;
  uint64_t esi;
  uint64_t ebx;
  uint64_t ebp;
  uint64_t return_eip;
} TaskReturnContext;

typedef enum TASK_STATE {
  TASK_STATE_DEAD = 0,
  TASK_STATE_READY = 1,
  TASK_STATE_IDLE = 2,
  TASK_STATE_WAITING_INPUT = 3,
  TASK_STATE_CREATED = 4, // just made by taskCreate()
  TASK_STATE_WAITING_CHILD = 5,
  TASK_STATE_WAITING_CHILD_SPECIFIC = 6, // task->waitingForPid
  TASK_STATE_WAITING_VFORK = 7,
  TASK_STATE_BLOCKED = 8,
  TASK_STATE_SIGKILLED = 9,
  TASK_STATE_FUTEX = 10,
  TASK_STATE_DUMMY = 69,
} TASK_STATE;

typedef struct KilledInfo {
  struct KilledInfo *next;

  uint64_t pid;
  uint16_t ret;
} KilledInfo;

typedef struct Task Task;

// task_info.c
typedef struct TaskInfoFs {
  Spinlock LOCK_FS;
  int      utilizedBy;

  char    *cwd;
  uint32_t umask;
} TaskInfoFs;

TaskInfoFs *taskInfoFsAllocate();
TaskInfoFs *taskInfoFsClone(TaskInfoFs *old);
void        taskInfoFsDiscard(TaskInfoFs *target);

typedef struct TaskInfoPagedir {
  Spinlock LOCK_PD;
  int      utilizedBy;

  uint64_t heap_start;
  uint64_t heap_end;

  uint64_t mmap_start;
  uint64_t mmap_end;

  uint64_t *pagedir;
} TaskInfoPagedir;

TaskInfoPagedir *taskInfoPdAllocate(bool pagedir);
TaskInfoPagedir *taskInfoPdClone(TaskInfoPagedir *old);
void             taskInfoPdDiscard(TaskInfoPagedir *target);

typedef struct IntTimerInternal {
  uint64_t at;    // checked agains timerTicks (ms)
  uint64_t reset; // reset value (ms)
} IntTimerInternal;

typedef struct TaskInfoSignal {
  Spinlock LOCK_SIGNAL;
  int      utilizedBy;

  IntTimerInternal itimerReal; // ITIMER_REAL
  struct sigaction signals[_NSIG];
} TaskInfoSignal;

TaskInfoSignal *taskInfoSignalAllocate();
TaskInfoSignal *taskInfoSignalClone(TaskInfoSignal *old);
void            taskInfoSignalDiscard(TaskInfoSignal *target);

typedef struct TaskInfoFiles {
  SpinlockCnt WLOCK_FILES;
  int         utilizedBy;

  size_t rlimitFdsSoft;
  size_t rlimitFdsHard;

  uint8_t *fdBitmap;

  AVLheader *firstFile; // value of OpenFile*
} TaskInfoFiles;

TaskInfoFiles *taskInfoFilesAllocate();
// TaskInfoFiles *taskInfoFilesClone(TaskInfoFiles *old);
void taskInfoFilesDiscard(TaskInfoFiles *target, void *task);

typedef struct TaskSysInterrupted {
  struct TaskSysInterrupted *next;

  uint64_t number; // rax
} TaskSysInterrupted;

#define EXTRAS_DISABLE_FUTEX (1 << 0)

struct Task {
  uint64_t id;
  int      pgid;
  int      tgid;
  int      sid;
  bool     kernel_task;
  uint8_t  state;

  uint64_t waitingForPid; // wait4()

  AsmPassedInterrupt registers;
  // uint64_t          *pagedir;
  uint64_t whileTssRsp;
  uint64_t whileSyscallRsp;

  // needed for syscalls
  uint64_t *pagedirOverride;

  bool systemCallInProgress;
  bool schedPageFault;

  AsmPassedInterrupt *syscallRegs;
  uint64_t            syscallRsp;

  TaskSysInterrupted *firstSysIntr;
  // no need for a lock, only we access it directly

  // Useful to switch, for when TLS is available
  uint64_t fsbase;
  uint64_t gsbase;

  // PLEASE assert() it is zero after use and zero it manually if state changes
  // are involved. It WILL NOT care at which point a wakeup happens and how
  // disruptive the context can be!
  size_t forcefulWakeupTimeUnsafe;

  termios  term;
  uint32_t tmpRecV;
  int      kernelErrno;
  int      ctrlPty;
  void    *spinlockQueueEntry; // check on kill!

  char  *cmdline;
  size_t cmdlineLen;
  char  *execname; // elf executable name

  // Remember, this is per THREAD! It just gets cloned
  sigset_t sigBlockList;
  sigset_t sigPendingList;

  TaskInfoFs      *infoFs;
  TaskInfoPagedir *infoPd;
  TaskInfoFiles   *infoFiles;
  TaskInfoSignal  *infoSignals;

  __attribute__((aligned(16))) uint8_t fpuenv[512];
  uint32_t                             mxcsr;

  bool noInformParent;

  Spinlock    LOCK_CHILD_TERM;
  KilledInfo *firstChildTerminated;
  int         childrenTerminatedAmnt;
  int        *tidptr;

  uint64_t extras; // extra flags

  Task *parent;
  Task *next;
};

SpinlockCnt TASK_LL_MODIFY;

Task *firstTask;
Task *currentTask;

Task *dummyTask;

bool tasksInitiated;

typedef struct BlockedTask {
  struct BlockedTask *next;

  Task *task;
} BlockedTask;

typedef struct Blocking {
  // also needs a parent lock to be reliable! this is just for the LL
  Spinlock     LOCK_LL_BLOCKED;
  BlockedTask *firstBlockedTask;
} Blocking;

// needed for libraries that still depend on some sort of errno
// should be safe as it's per-thread
#define errno (currentTask->kernelErrno)

void taskBlock(Blocking *blocking, Task *task, Spinlock *releaseAfter,
               bool apply);
void taskUnblock(Blocking *blocking);
void taskSpinlockExit(Task *task, Spinlock *lock);

void initiateTasks();

Task *taskCreate(uint32_t id, uint64_t rip, bool kernel_task, uint64_t *pagedir,
                 uint32_t argc, char **argv);
Task *taskCreateKernel(uint64_t rip, uint64_t rdi);
void  taskNameKernel(Task *target, const char *str, int len);
void  taskCreateFinish(Task *task);

void taskAdjustHeap(Task *task, size_t new_heap_end, size_t *start,
                    size_t *end);

Task *taskGet(uint32_t id);
void  taskKill(uint32_t id, uint16_t ret);
void  taskFreeChildren(Task *task);

size_t taskChangeCwd(char *newdir);
Task  *taskFork(AsmPassedInterrupt *cpu, uint64_t rsp, int cloneFlags,
                bool spinup);
void   taskFilesCopy(Task *original, Task *target, bool respectCOE);

Task *taskListAllocate();
void  taskListDestroy(Task *target);

uint64_t taskGenerateId();
void     taskCallReaper(Task *target);

#endif
