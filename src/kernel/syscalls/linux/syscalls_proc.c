#include <acpi.h>
#include <bootloader.h>
#include <elf.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

// process lifetime system calls (send help)

#define SYSCALL_PIPE 22
static size_t syscallPipe(int *fds) { return pipeOpen(fds); }

#define SYSCALL_PIPE2 293
static size_t syscallPipe2(int *fds, int flags) {
  if (flags && (flags & ~(O_CLOEXEC | O_NONBLOCK)) != 0) {
    dbgSysStubf("todo flags");
    return ERR(ENOSYS);
  }

  size_t out = pipeOpen(fds);
  if (out < 0)
    goto cleanup;

  if (flags) {
    // since basically the only one we support atm is the close-on-exec flag xd
    OpenFile *fd0 = fsUserGetNode(currentTask, fds[0]);
    OpenFile *fd1 = fsUserGetNode(currentTask, fds[1]);

    if (!fd0 || !fd1) {
      debugf("[syscalls::pipe2] Bad sync!\n");
      panic();
    }

    if (flags & O_CLOEXEC) {
      fd0->closeOnExec = true;
      fd1->closeOnExec = true;
    }
    if (flags & O_NONBLOCK) {
      fd0->flags |= O_NONBLOCK;
      fd1->flags |= O_NONBLOCK;
    }
  }

cleanup:
  return out;
}

#define SYSCALL_CLONE 56
static size_t syscallClone(uint64_t flags, uint64_t newsp, int *parent_tid,
                           int *child_tid, uint64_t tls) {
  // 17 is SIGCHLD which we ignore
  // CLONE_DETACHED, CLONE_SYSVSEM(*) are also ignored
  uint64_t supported_flags =
      CLONE_VFORK | CLONE_VM | CLONE_FILES | CLONE_SYSVSEM |
      CLONE_CHILD_CLEARTID | CLONE_PARENT_SETTID | CLONE_DETACHED |
      CLONE_THREAD | CLONE_SETTLS | CLONE_FS | CLONE_SIGHAND | 17;
  if ((flags & ~supported_flags) != 0) {
    dbgSysStubf("todo more flags %lx", (flags & ~supported_flags));
    return ERR(ENOSYS);
  }

  if (flags & CLONE_VFORK) {
    // those are assumed! a lot of software forgets CLONE_FILES..
    flags |= CLONE_VM;
    flags |= CLONE_FILES;
  }

  Task *newTask =
      taskFork(currentTask->syscallRegs,
               newsp ? newsp : currentTask->syscallRsp, flags, false);
  uint64_t id = newTask->id;

  if (flags & CLONE_SETTLS)
    newTask->fsbase = tls;

  if (flags & CLONE_CHILD_CLEARTID)
    newTask->tidptr = child_tid;

  if (flags & CLONE_PARENT_SETTID)
    *parent_tid = newTask->id;

  // no race condition today :")
  taskCreateFinish(newTask);

  // potential race condition here and on the respective one below
  if (flags & CLONE_VFORK) {
    currentTask->state = TASK_STATE_WAITING_VFORK;
    handControl();
  }

  return id;
}

#define SYSCALL_FORK 57
static size_t syscallFork() {
  return taskFork(currentTask->syscallRegs, currentTask->syscallRsp, 0, true)
      ->id;
}

#define SYSCALL_VFORK 58
static size_t syscallVfork() {
  Task *newTask = taskFork(currentTask->syscallRegs, currentTask->syscallRsp,
                           CLONE_VM, false);
  int   id = newTask->id;

  // no race condition today :")
  taskCreateFinish(newTask);

  currentTask->state = TASK_STATE_WAITING_VFORK;
  handControl();

  return id;
}

typedef struct CopyPtrStyle {
  int      count;
  char   **ptrPlace;
  uint8_t *valPlace;
} CopyPtrStyle;

CopyPtrStyle copyPtrStyle(char **ptr) {
  int    count = 0;
  size_t totalLen = 0;
  while (ptr[count]) {
    totalLen += strlength(ptr[count]) + 1;
    count++;
  }
  char   **ptrPlace = malloc(count * sizeof(void *));
  uint8_t *valPlace = malloc(totalLen);
  size_t   curr = 0;
  for (int i = 0; i < count; i++) {
    ptrPlace[i] = (void *)((size_t)valPlace + curr);
    curr += strlength(ptr[i]) + 1;
    memcpy(ptrPlace[i], ptr[i], strlength(ptr[i]) + 1);
  }
  CopyPtrStyle ret = {
      .count = count, .ptrPlace = ptrPlace, .valPlace = valPlace};
  return ret;
}

void freeItemsIfNeeded(char **argv) {
  int index = 0;
  while (argv[index]) {
    char *backing = argv[index];
    if (IS_INSIDE_HHDM(backing)) {
      free(backing);
      argv[index] = 0; // should be a fake argv, thus no userspace interference
    }
    index++;
  }
}

void freeArgvIfNeeded(char **argv) {
  if (IS_INSIDE_HHDM(argv)) {
    // previous was a shebang
    free(argv);
  }
}

#define SYSCALL_EXECVE 59
static size_t syscallExecve(char *filename, char **argv, char **envp) {
  assert(argv[0]); // shebang support depends on it atm. check (dep)
  dbgSysExtraf("filename{%s}", filename);
  spinlockAcquire(&currentTask->infoFs->LOCK_FS);
  char *filenameSanitized = fsSanitize(currentTask->infoFs->cwd, filename);
  spinlockRelease(&currentTask->infoFs->LOCK_FS);
  uint8_t *buff = calloc(256, 1);

  // do a read to check for alternatives, elfExecute() still does its checks
  OpenFile *preScan = fsKernelOpen(filenameSanitized, O_RDONLY, 0);
  if (!preScan) {
    free(filenameSanitized);
    return ERR(ENOENT);
  }
  size_t max = fsRead(preScan, buff, 255); // 1 less so there's always a \0
  fsKernelClose(preScan);
  if (RET_IS_ERR(max)) {
    free(filenameSanitized);
    return max;
  }

  if (max > 2 && buff[0] == '#' && buff[1] == '!') {
    // shebang detected!
    char *arg2 = (char *)0;
    int   spaces = 0;
    for (int i = 0; i < max; i++) {
      if (buff[i] == ' ') {
        if (spaces == 0) {
          buff[i] = '\0'; // needs to be null terminated
          arg2 = (char *)&buff[i + 1];
        }
        spaces++;
      } else if (buff[i] == '\n') {
        buff[i] = '\0'; // for the last arg (if none, 256-255=1 calloc)
        break;          // no longer needed + don't risk above
      }
    }
    int argc = 0;
    while (argv[argc++])
      ; // why not just pass argc. truly beyond me...
    int    amnt = arg2 ? 2 : 1;
    char **injectedArgv = calloc((amnt + argc + 1) * sizeof(char *), 1);
    memcpy(&injectedArgv[amnt], argv, argc * sizeof(char *));
    injectedArgv[amnt] = strdup(filename); // replace [0] w/original (dep)

    freeArgvIfNeeded(argv); // ! don't use argv anymore

    char *arg1 = (char *)&buff[2];
    if (arg2 && arg2[0] != '\0')
      injectedArgv[1] = strdup(arg2);
    injectedArgv[0] = strdup(arg1);

    free(buff); // ! don't use anything defined before after this
    syscallExecve(injectedArgv[0], injectedArgv, envp);
    assert(false); // won't return, injectedArgv & buff are above HHDM
  } else if (max > 4 && buff[EI_MAG0] == ELFMAG0 && buff[EI_MAG1] == ELFMAG1 &&
             buff[EI_MAG2] == ELFMAG2 && buff[EI_MAG3] == ELFMAG3) {
    // standard elf executable, go on
  } else {
    // both unnecessary, won't do anything w/them :^)
    freeItemsIfNeeded(argv); // depends on the argv so NEEDS to be first
    freeArgvIfNeeded(argv);
    return ERR(ENOEXEC);
  }

  CopyPtrStyle arguments = copyPtrStyle(argv);
  CopyPtrStyle environment = copyPtrStyle(envp);

  Task *ret = elfExecute(filenameSanitized, arguments.count, arguments.ptrPlace,
                         environment.count, environment.ptrPlace, 0);
  free(filenameSanitized);
  free(arguments.ptrPlace);
  free(arguments.valPlace);
  free(environment.ptrPlace);
  free(environment.valPlace);
  freeItemsIfNeeded(argv); // depends on the argv so NEEDS to be first
  freeArgvIfNeeded(argv);  // ! don't use ANY argv after this
  if (!ret)
    return ERR(ENOENT);

  int targetId = currentTask->id;
  currentTask->id = taskGenerateId();
  int targetTgid = currentTask->tgid;
  currentTask->tgid = currentTask->id; // better way to do alladat

  ret->id = targetId;
  ret->tgid = targetTgid;
  ret->parent = currentTask->parent;
  ret->pgid = currentTask->pgid;
  ret->sid = currentTask->sid;
  ret->ctrlPty = currentTask->ctrlPty;
  ret->sigBlockList = currentTask->sigBlockList;
  taskInfoFsDiscard(ret->infoFs);
  ret->infoFs = taskInfoFsClone(currentTask->infoFs);
  ret->infoSignals = taskInfoSignalClone(currentTask->infoSignals);
  for (int i = 0; i < _NSIG; i++) {
    // restore non-ignored handlers to defaults
    if (ret->infoSignals->signals[i].sa_handler != SIG_IGN)
      ret->infoSignals->signals[i].sa_handler = SIG_DFL;
  }

  taskFilesCopy(currentTask, ret, true);

  taskCreateFinish(ret);
  if (currentTask->parent->state == TASK_STATE_WAITING_VFORK)
    currentTask->parent->state = TASK_STATE_READY;

  currentTask->noInformParent = true;
  taskKill(currentTask->id, 0);
  return 0; // will never be reached, it **replaces**
}

#define SYSCALL_EXIT_TASK 60
static void syscallExitTask(int return_code) {
  // if (return_code)
  //   panic();
  taskKill(currentTask->id, return_code);
}

#define SYSCALL_WAIT4 61
static size_t syscallWait4(int pid, int *wstatus, int options,
                           struct rusage *ru) {
  if (options || ru)
    dbgSysStubf("todo options & rusage");
  /*dbgSysExtraf("WNOHANG{%d} WUNTRACED{%d} "
               "WSTOPPED{%d} WEXITED{%d} WCONTINUED{%d} "
               "WNOWAIT{%d}",
               options & WNOHANG, options & WUNTRACED, options & WSTOPPED,
               options & WEXITED, options & WCONTINUED, options & WNOWAIT);*/

  asm volatile("sti");

  // if nothing is on the side, then ensure we have something to wait() for
  if (!currentTask->childrenTerminatedAmnt) {
    int amnt = 0;

    spinlockCntReadAcquire(&TASK_LL_MODIFY);
    Task *browse = firstTask;
    while (browse) {
      if (browse->state == TASK_STATE_READY && browse->parent == currentTask)
        amnt++;
      browse = browse->next;
    }
    spinlockCntReadRelease(&TASK_LL_MODIFY);

    if (!amnt)
      return ERR(ECHILD);
  }

  // target is the item we "found"
  KilledInfo *target = 0;

  // check if specific pid item is already there
  if (pid != -1) {
    spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
    KilledInfo *browse = currentTask->firstChildTerminated;
    while (browse) {
      if (browse->pid == pid)
        break;
      browse = browse->next;
    }
    target = browse;
    spinlockRelease(&currentTask->LOCK_CHILD_TERM);

    // not there? wait for it!
    if (!target) {
      if (options & WNOHANG)
        return 0;
      // "poll"
      currentTask->waitingForPid = pid;
      currentTask->state = TASK_STATE_WAITING_CHILD_SPECIFIC;
      handControl();
      if (signalsPendingQuick(currentTask))
        return ERR(EINTR);

      // we're back
      currentTask->waitingForPid = 0; // just for good measure
      spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
      KilledInfo *browse = currentTask->firstChildTerminated;
      while (browse) {
        if (browse->pid == pid)
          break;
        browse = browse->next;
      }
      target = browse;
      spinlockRelease(&currentTask->LOCK_CHILD_TERM);
    }
  } else {
    // todo: process group stuff! also pid vs tgid!
    // we got children, wait for any changes
    // OR just continue :")
    if (!currentTask->childrenTerminatedAmnt) {
      if (options & WNOHANG)
        return 0;
      currentTask->state = TASK_STATE_WAITING_CHILD;
      handControl();
      if (signalsPendingQuick(currentTask))
        return ERR(EINTR);
    }
    target = currentTask->firstChildTerminated;
  }

  spinlockAcquire(&currentTask->LOCK_CHILD_TERM);
  if (!target) {
    debugf("[syscalls::wait4] FATAL Just fatal!");
    panic();
  }

  int output = target->pid;
  int ret = target->ret;

  // cleanup
  LinkedListRemove((void **)(&currentTask->firstChildTerminated), target);
  currentTask->childrenTerminatedAmnt--;
  spinlockRelease(&currentTask->LOCK_CHILD_TERM);

  if (wstatus) {
    if (ret < 128)
      *wstatus = (ret & 0xff) << 8;
    else {
      int sig = ret - 128;
      *wstatus = (sig & 0xff);
    }
  }

  dbgSysExtraf("\n%d [RET] [syscall::wait4] pid{%d} ret{%d}", currentTask->id,
               output, ret);
  return output;
}

#define SYSCALL_REBOOT 169
static size_t syscallReboot(int magic1, int magic2, uint32_t cmd, void *arg) {
  if (magic1 != LINUX_REBOOT_MAGIC1 || magic2 != LINUX_REBOOT_MAGIC2)
    return ERR(EINVAL);
  switch (cmd) {
  case LINUX_REBOOT_CMD_POWER_OFF:
    return acpiPoweroff();
    break;
  case LINUX_REBOOT_CMD_RESTART:
    return acpiReboot();
    break;
  default:
    return ERR(EINVAL);
    break;
  }
}

#define SYSCALL_EXIT_GROUP 231
static void syscallExitGroup(int return_code) {
  spinlockCntReadAcquire(&TASK_LL_MODIFY);
  Task *browse = firstTask;
  while (browse) {
    if (browse->tgid == currentTask->tgid && browse->id != currentTask->id) {
      // found one of ours!
      atomicBitmapSet(&browse->sigPendingList, SIGKILL);
    }
    browse = browse->next;
  }
  spinlockCntReadRelease(&TASK_LL_MODIFY);
  syscallExitTask(return_code);
}

#define SYSCALL_EVENTFD2 290
static size_t syscallEventfd2(uint64_t initValue, int flags) {
  return eventFdOpen(initValue, flags);
}

void syscallsRegProc() {
  registerSyscall(SYSCALL_PIPE, syscallPipe);
  registerSyscall(SYSCALL_PIPE2, syscallPipe2);
  registerSyscall(SYSCALL_EXIT_TASK, syscallExitTask);
  registerSyscall(SYSCALL_CLONE, syscallClone);
  registerSyscall(SYSCALL_FORK, syscallFork);
  registerSyscall(SYSCALL_VFORK, syscallVfork);
  registerSyscall(SYSCALL_WAIT4, syscallWait4);
  registerSyscall(SYSCALL_EXECVE, syscallExecve);
  registerSyscall(SYSCALL_EXIT_GROUP, syscallExitGroup);
  registerSyscall(SYSCALL_REBOOT, syscallReboot);
  registerSyscall(SYSCALL_EVENTFD2, syscallEventfd2);
}
