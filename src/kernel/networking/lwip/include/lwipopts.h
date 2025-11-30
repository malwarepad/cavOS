#include "arch/cc.h"

// extending on lwip/include/lwip/opt.h

#ifndef CUSTOM_LWIPOPTS_H
#define CUSTOM_LWIPOPTS_H

#define LWIP_PROVIDE_ERRNO 1
#define MEM_LIBC_MALLOC 1
#define LWIP_NO_CTYPE_H 1
#define LWIP_NO_UNISTD_H 1
#define __DEFINED_ssize_t 1
#define LWIP_NO_LIMITS_H 1
#define LWIP_TIMEVAL_PRIVATE 0
#define CUSTOM_IOVEC 1
#define LWIP_SO_RCVBUF 1
#define LWIP_NETCONN_FULLDUPLEX 1
#define LWIP_NETCONN_SEM_PER_THREAD 1

#define TCPIP_MBOX_SIZE 32

#define LWIP_RAW 1
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_DEBUG 1

// raise connection limits
#define MEMP_NUM_NETCONN 100
#define MEMP_NUM_TCP_PCB 100

// raise the server buffer (forced to do second)
#define TCP_SND_BUF 8192
#define MEMP_NUM_TCP_SEG (2 * TCP_SND_QUEUELEN)

// optimizations
#define TCP_WND (16 * TCP_MSS)
#define LWIP_CHKSUM_ALGORITHM 3

#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_COMPAT_SOCKETS 0

typedef struct mboxBlock {
  LLheader _ll;

  Task *task;
  bool  write;
} mboxBlock;

typedef struct {
  Spinlock LOCK;

  LLcontrol firstBlock; // struct mboxBlock

  bool   invalid;
  int    ptrRead;
  int    ptrWrite;
  int    size;
  void **msges;
} sys_mbox_t;

typedef uint64_t  sys_thread_t;
typedef Semaphore sys_sem_t;
typedef Spinlock  sys_mutex_t;
// typedef lwip_mbox sys_mbox_t;

// #define SYS_LIGHTWEIGHT_PROT 1
typedef uint8_t sys_prot_t;

#define LWIP_PLATFORM_ASSERT(x)                                                \
  do {                                                                         \
    debugf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__,          \
           __FILE__);                                                          \
    panic();                                                                   \
  } while (0)

#define LWIP_PLATFORM_DIAG(x)                                                  \
  do {                                                                         \
    debugf x;                                                                  \
  } while (0)

#define LWIP_RAND() ((u32_t)rand())

// off lwip/include/lwip/sys.h

// #define sys_sem_new(s, c) ERR_OK
// #define sys_sem_signal(s)
// #define sys_sem_wait(s)
// #define sys_arch_sem_wait(s, t)
// #define sys_sem_free(s)
// #define sys_sem_valid(s) 0
// #define sys_sem_valid_val(s) 0
// #define sys_sem_set_invalid(s)
// #define sys_sem_set_invalid_val(s)
// #define sys_mutex_new(mu) ERR_OK
// #define sys_mutex_lock(mu)
// #define sys_mutex_unlock(mu)
// #define sys_mutex_free(mu)
// #define sys_mutex_valid(mu) 0
// #define sys_mutex_set_invalid(mu)
// #define sys_mbox_new(m, s) ERR_OK
// #define sys_mbox_fetch(m, d)
// #define sys_mbox_tryfetch(m, d)
// #define sys_mbox_post(m, d)
// #define sys_mbox_trypost(m, d)
// #define sys_mbox_free(m)
// #define sys_mbox_valid(m)
// #define sys_mbox_valid_val(m)
// #define sys_mbox_set_invalid(m)
// #define sys_mbox_set_invalid_val(m)

// #define sys_thread_new(n, t, a, s, p)

// #define sys_msleep(t)

#endif