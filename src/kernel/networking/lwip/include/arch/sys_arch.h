#include "cc.h"
#include "lwip/err.h"

void sys_init(void);

uint32_t     sys_now(void);
sys_thread_t sys_thread_new(const char *pcName,
                            void (*pxThread)(void *pvParameters), void *pvArg,
                            int iStackSize, int iPriority);

void  sys_mutex_lock(Spinlock *spinlock);
void  sys_mutex_unlock(Spinlock *spinlock);
err_t sys_mutex_new(Spinlock *spinlock);

err_t    sys_sem_new(sys_sem_t *sem, uint8_t cnt);
void     sys_sem_signal(sys_sem_t *sem);
uint32_t sys_arch_sem_wait(sys_sem_t *sem, uint32_t timeout);
void     sys_sem_free(sys_sem_t *sem);
void     sys_sem_set_invalid(sys_sem_t *sem);
int      sys_sem_valid(sys_sem_t *sem);

err_t      LWIP_NETCONN_THREAD_SEM_ALLOC();
err_t      LWIP_NETCONN_THREAD_SEM_FREE();
sys_sem_t *LWIP_NETCONN_THREAD_SEM_GET();

err_t sys_mbox_new(sys_mbox_t *mbox, int size);
void  sys_mbox_free(sys_mbox_t *mbox);
void  sys_mbox_set_invalid(sys_mbox_t *mbox);
int   sys_mbox_valid(sys_mbox_t *mbox);
void  sys_mbox_post(sys_mbox_t *q, void *msg);
err_t sys_mbox_trypost(sys_mbox_t *q, void *msg);
err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg);
u32_t sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout);
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg);

void *sio_open(u8_t devnum);
u32_t sio_write(void *fd, const u8_t *data, u32_t len);
void  sio_send(u8_t c, void *fd);
u8_t  sio_recv(void *fd);
u32_t sio_read(void *fd, u8_t *data, u32_t len);
u32_t sio_tryread(void *fd, u8_t *data, u32_t len);
