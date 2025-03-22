#include <linux.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>

#define SYSCALL_GETPID 39
static size_t syscallGetPid() { return currentTask->id; }

#define SYSCALL_GETCWD 79
static size_t syscallGetcwd(char *buff, size_t size) {
  spinlockAcquire(&currentTask->infoFs->LOCK_FS);
  size_t realLength = strlength(currentTask->infoFs->cwd) + 1;
  if (size < realLength) {
    spinlockRelease(&currentTask->infoFs->LOCK_FS);
    return ERR(ERANGE);
  }
  memcpy(buff, currentTask->infoFs->cwd, realLength);

  spinlockRelease(&currentTask->infoFs->LOCK_FS);
  return realLength;
}

// Beware the 65 character limit!
char sysname[] = "Cave-Like Operating System";
char nodename[] = "cavOS";
char release[] = "0.69.2";
char version[] = "0.69.2";
char machine[] = "x86_64";

#define SYSCALL_UNAME 63
static size_t syscallUname(struct old_utsname *utsname) {
  memcpy(utsname->sysname, sysname, sizeof(sysname));
  memcpy(utsname->nodename, nodename, sizeof(nodename));
  memcpy(utsname->release, release, sizeof(release));
  memcpy(utsname->version, version, sizeof(version));
  memcpy(utsname->machine, machine, sizeof(machine));

  return 0;
}

#define SYSCALL_CHDIR 80
static size_t syscallChdir(char *newdir) {
  dbgSysExtraf("newdir{%s}", newdir);
  return taskChangeCwd(newdir);
}

#define SYSCALL_FCHDIR 81
static size_t syscallFchdir(int fd) {
  OpenFile *file = fsUserGetNode(currentTask, fd);
  if (!file)
    return ERR(EBADF);
  if (!file->dirname)
    return ERR(ENOTDIR);
  return syscallChdir(file->dirname);
}

struct rlimit {
  size_t rlim_cur; /* Soft limit */
  size_t rlim_max; /* Hard limit (ceiling for rlim_cur) */
};

#define SYSCALL_GETRLIMIT 97
static size_t syscallGetrlimit(int resource, struct rlimit *rlim) {
  switch (resource) {
  case 7: // max open fds
    rlim->rlim_cur = UINT64_MAX - 69;
    rlim->rlim_max = UINT64_MAX - 69;
    // todo: be realistic about this and have a bitmap or smth
    return 0;
    break;
  default:
    return ERR(ENOSYS);
    break;
  }
}

#define SYSCALL_GETUID 102
static size_t syscallGetuid() {
  return 0; // root ;)
}

#define SYSCALL_GETGID 104
static size_t syscallGetgid() {
  return 0; // root ;)
}

#define SYSCALL_SETUID 105
static size_t syscallSetuid(size_t uid) {
  if (uid != 0) // no getting out of root for you :^)
    return ERR(EPERM);
  return 0;
}

#define SYSCALL_SETGID 106
static size_t syscallSetgid(size_t gid) {
  if (gid != 0) // no getting out of root for you :^)
    return ERR(EPERM);
  return 0;
}

#define SYSCALL_GETEUID 107
static size_t syscallGeteuid() {
  return 0; // root ;)
}

#define SYSCALL_GETEGID 108
static size_t syscallGetegid() {
  return 0; // root ;)
}

#define SYSCALL_SETPGID 109
static size_t syscallSetpgid(int pid, int pgid) {
  if (!pid)
    pid = currentTask->id;

  Task *task = taskGet(pid);
  if (!task) {
    dbgSysExtraf("no such task w/pid{%d}", pid);
    return ERR(EPERM);
  }

  task->pgid = pgid;
  return 0;
}

#define SYSCALL_GETPPID 110
static size_t syscallGetppid() {
  if (currentTask->parent)
    return currentTask->parent->id;
  else
    return KERNEL_TASK_ID;
}

#define SYSCALL_GETGROUPS 115
static size_t syscallGetgroups(int gidsetsize, uint32_t *gids) {
  if (!gidsetsize)
    return 1;

  gids[0] = 0;
  return 1;
}

#define SYSCALL_GETPGID 121
static size_t syscallGetpgid() { return currentTask->pgid; }

#define SYSCALL_PRCTL 158
static size_t syscallPrctl(int code, size_t addr) {
  switch (code) {
  case 0x1002:
    currentTask->fsbase = addr;
    wrmsr(MSRID_FSBASE, currentTask->fsbase);

    return 0;
    break;
  }

  dbgSysStubf("unsupported code{%d:0x%x}", code, code);
  return ERR(ENOSYS);
}

#define SYSCALL_GET_TID 186
static size_t syscallGetTid() { return currentTask->id; }

#define SYSCALL_SET_TID_ADDR 218
static size_t syscallSetTidAddr(int *tidptr) {
  // todo: tidptr + futex() WAKEUP!
  return currentTask->id;
}

// todo.. actually random!
#define SYSCALL_GETRANDOM 318
static size_t syscallGetRandom(char *buff, size_t count, uint32_t flags) {
  srand(timerTicks);
  for (int i = 0; i < count; i++)
    buff[i] = rand();
  return count;
}

void syscallsRegEnv() {
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_GETCWD, syscallGetcwd);
  registerSyscall(SYSCALL_CHDIR, syscallChdir);
  registerSyscall(SYSCALL_GETRLIMIT, syscallGetrlimit);
  registerSyscall(SYSCALL_GETUID, syscallGetuid);
  registerSyscall(SYSCALL_GETEUID, syscallGeteuid);
  registerSyscall(SYSCALL_GETGID, syscallGetgid);
  registerSyscall(SYSCALL_GETEGID, syscallGetegid);
  registerSyscall(SYSCALL_GETPPID, syscallGetppid);
  registerSyscall(SYSCALL_GETPGID, syscallGetpgid);
  registerSyscall(SYSCALL_SETPGID, syscallSetpgid);
  registerSyscall(SYSCALL_SETUID, syscallSetuid);
  registerSyscall(SYSCALL_SETGID, syscallSetgid);
  registerSyscall(SYSCALL_PRCTL, syscallPrctl);
  registerSyscall(SYSCALL_SET_TID_ADDR, syscallSetTidAddr);
  registerSyscall(SYSCALL_GET_TID, syscallGetTid);
  registerSyscall(SYSCALL_UNAME, syscallUname);
  registerSyscall(SYSCALL_FCHDIR, syscallFchdir);
  registerSyscall(SYSCALL_GETGROUPS, syscallGetgroups);
  registerSyscall(SYSCALL_GETRANDOM, syscallGetRandom);
}
