#include <linux.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <util.h>

#define SYSCALL_GETPID 39
static uint32_t syscallGetPid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getpid] pid{%d}\n", currentTask->id);
#endif
  return currentTask->id;
}

#define SYSCALL_GETCWD 79
static int syscallGetcwd(char *buff, size_t size) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getcwd] buff{%lx} size{%lx}\n", buff, size);
#endif
  size_t realLength = strlength(currentTask->cwd) + 1;
  if (size < realLength) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::getcwd] FAIL! Not enough space on buffer! given{%ld} "
           "required{%ld}\n",
           size, realLength);
#endif
    return -1;
  }
  memcpy(buff, currentTask->cwd, realLength);

  return realLength;
}

// Beware the 65 character limit!
char sysname[] = "Cave-Like Operating System";
char nodename[] = "cavOS";
char release[] = "0.69.2";
char version[] = "0.69.2";
char machine[] = "x86_64";

#define SYSCALL_UNAME 63
static int syscallUname(struct old_utsname *utsname) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::uname] utsname{%lx}\n", utsname);
#endif

  memcpy(utsname->sysname, sysname, sizeof(sysname));
  memcpy(utsname->nodename, nodename, sizeof(nodename));
  memcpy(utsname->release, release, sizeof(release));
  memcpy(utsname->version, version, sizeof(version));
  memcpy(utsname->machine, machine, sizeof(machine));

  return 0;
}

#define SYSCALL_CHDIR 80
static int syscallChdir(char *newdir) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::chdir] place{%s}\n", newdir);
#endif
  bool ret = taskChangeCwd(newdir);
  if (!ret)
    return -1;

#if DEBUG_SYSCALLS_FAILS
  debugf("[syscalls::chdir] FAIL! Tried to change to %s!\n", newdir);
#endif
  return 0;
}

#define SYSCALL_GETUID 102
static int syscallGetuid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getuid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETGID 104
static int syscallGetgid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getgid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETEUID 107
static int syscallGeteuid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::geteuid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_GETEGID 108
static int syscallGetegid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getegid]\n");
#endif
  return 0; // root ;)
}

#define SYSCALL_SETPGID 109
static int syscallSetpgid(int pid, int pgid) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::setpgid] pid{%d} pgid{%d}\n", pid, pgid);
#endif

  if (!pid)
    pid = currentTask->id;

  Task *task = taskGet(pid);
  if (!task) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::setpgid] FAIL! Couldn't find task. pid{%d}\n", pid);
#endif
    return -1;
  }

  task->pgid = pgid;
  return 0;
}

#define SYSCALL_GETPGID 121
static int syscallGetpgid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::getpgid]\n");
#endif
  return currentTask->pgid;
}

#define SYSCALL_PRCTL 158
static int syscallPrctl(int code, size_t addr) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::prctl] code{%d} addr{%lx}!\n", code, addr);
#endif

  switch (code) {
  case 0x1002:
    currentTask->fsbase = addr;
    wrmsr(MSRID_FSBASE, currentTask->fsbase);

    return 0;
    break;
  }

  debugf("[syscalls::prctl] Unsupported code! code_10{%d} code_16{%x}!\n", code,
         code);
  return -1;
}

#define SYSCALL_GET_TID 186
static int syscallGetTid() {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::gettid]\n");
#endif
  return currentTask->id;
}

#define SYSCALL_SET_TID_ADDR 218
static int syscallSetTidAddr(int *tidptr) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::settid] tidptr{%lx}\n", tidptr);
#endif
  *tidptr = currentTask->id;
  return currentTask->id;
}

void syscallsRegEnv() {
  registerSyscall(SYSCALL_GETPID, syscallGetPid);
  registerSyscall(SYSCALL_GETCWD, syscallGetcwd);
  registerSyscall(SYSCALL_CHDIR, syscallChdir);
  registerSyscall(SYSCALL_GETUID, syscallGetuid);
  registerSyscall(SYSCALL_GETEUID, syscallGeteuid);
  registerSyscall(SYSCALL_GETGID, syscallGetgid);
  registerSyscall(SYSCALL_GETEGID, syscallGetegid);
  registerSyscall(SYSCALL_GETPGID, syscallGetpgid);
  registerSyscall(SYSCALL_SETPGID, syscallSetpgid);
  registerSyscall(SYSCALL_PRCTL, syscallPrctl);
  registerSyscall(SYSCALL_SET_TID_ADDR, syscallSetTidAddr);
  registerSyscall(SYSCALL_GET_TID, syscallGetTid);
  registerSyscall(SYSCALL_UNAME, syscallUname);
}
