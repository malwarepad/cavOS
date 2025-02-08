#include <bootloader.h>
#include <malloc.h>
#include <pci.h>
#include <task.h>
#include <timer.h>
#include <types.h>
#include <uacpi/uacpi.h>
#include <util.h>

#include <stdatomic.h>

// Bad (quick & dirty) uACPI glue code
// Copyright (C) 2024 Panagiotis

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
  *out_rsdp_address = bootloader.rsdp;
  return UACPI_STATUS_OK;
}

void uacpi_kernel_log(uacpi_log_level lvl, const uacpi_char *str) {
  const char *lvlstr;

  switch (lvl) {
  case UACPI_LOG_DEBUG:
    lvlstr = "debug";
    break;
  case UACPI_LOG_TRACE:
    lvlstr = "trace";
    break;
  case UACPI_LOG_INFO:
    lvlstr = "info";
    break;
  case UACPI_LOG_WARN:
    lvlstr = "warn";
    break;
  case UACPI_LOG_ERROR:
    lvlstr = "error";
    break;
  default:
    lvlstr = "invalid";
  }

  debugf("[acpi::%s] %s", lvlstr, str);
}

void *uacpi_kernel_alloc(uacpi_size size) { return malloc(size); }
void  uacpi_kernel_free(void *mem) { free(mem); }

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
  if (!currentTask)
    return 0;
  return (uacpi_thread_id)currentTask->id;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
                                 uacpi_handle *out_handle) {
  *out_handle = (uacpi_handle)base;
  return UACPI_STATUS_OK;
}
void uacpi_kernel_io_unmap(uacpi_handle handle) { (void)0; }

uacpi_status uacpi_kernel_io_read(uacpi_handle handle, uacpi_size offset,
                                  uacpi_u8 width, uacpi_u64 *out) {
  uacpi_io_addr target = (uacpi_io_addr)handle + offset;
  switch (width) {
  case 1:
    *out = inportb(target);
    break;
  case 2:
    *out = inportw(target);
    break;
  case 4:
    *out = inportl(target);
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset,
                                   uacpi_u8 *out_value) {
  return uacpi_kernel_io_read(handle, offset, 1, (uacpi_u64 *)out_value);
}
uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset,
                                    uacpi_u16 *out_value) {
  return uacpi_kernel_io_read(handle, offset, 2, (uacpi_u64 *)out_value);
}
uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset,
                                    uacpi_u32 *out_value) {
  return uacpi_kernel_io_read(handle, offset, 4, (uacpi_u64 *)out_value);
}

uacpi_status uacpi_kernel_io_write(uacpi_handle handle, uacpi_size offset,
                                   uacpi_u8 width, uacpi_u64 value) {
  uacpi_io_addr target = (uacpi_io_addr)handle + offset;
  switch (width) {
  case 1:
    outportb(target, value);
    break;
  case 2:
    outportw(target, value);
    break;
  case 4:
    outportl(target, value);
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset,
                                    uacpi_u8 in_value) {
  return uacpi_kernel_io_write(handle, offset, 1, in_value);
}
uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset,
                                     uacpi_u16 in_value) {
  return uacpi_kernel_io_write(handle, offset, 2, in_value);
}
uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset,
                                     uacpi_u32 in_value) {
  return uacpi_kernel_io_write(handle, offset, 4, in_value);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
  return (timerBootUnix * 1000 * 1000000) + (timerTicks * 1000000);
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle) {
  irqHandler *irqHand = registerIRQhandler(irq, handler);
  irqHand->argument = (size_t)ctx;
  *out_irq_handle = irqHand;
  return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler,
                                         uacpi_handle            irq_handle) {
  debugf("handler remove! todo!\n");
  panic();
  return UACPI_STATUS_OK;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
  Spinlock *spinlock = (Spinlock *)calloc(sizeof(Spinlock), 1);
  return spinlock;
}
void uacpi_kernel_free_spinlock(uacpi_handle spinlock) { free(spinlock); }
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle lock) {
  int intState = checkInterrupts();
  spinlockAcquire(lock);
  return intState;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle lock, uacpi_cpu_flags intstate) {
  spinlockRelease(lock);
  if (intstate)
    asm volatile("sti");
  else
    asm volatile("cli");
}

uacpi_handle uacpi_kernel_create_mutex(void) {
  return uacpi_kernel_create_spinlock();
}
void uacpi_kernel_free_mutex(uacpi_handle mutex) {
  return uacpi_kernel_free_spinlock(mutex);
}
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle mut, uacpi_u16 time) {
  if (time == 0xFFFF) {
    spinlockAcquire(mut);
    return UACPI_STATUS_OK;
  }

  size_t start = timerTicks;
  while (atomic_flag_test_and_set_explicit((Spinlock *)mut,
                                           memory_order_acquire)) {
    if (timerTicks > (start + time))
      return UACPI_STATUS_TIMEOUT;
    handControl();
  }
  return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle mut) { spinlockRelease(mut); }

uacpi_handle uacpi_kernel_create_event(void) {
  Semaphore *sem = (Semaphore *)calloc(sizeof(Semaphore), 1);
  return sem;
}
void       uacpi_kernel_free_event(uacpi_handle sem) { free(sem); }
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle sem, uacpi_u16 timeout) {
  return semaphoreWait(sem, timeout);
}
void uacpi_kernel_signal_event(uacpi_handle sem) { semaphorePost(sem); }
void uacpi_kernel_reset_event(uacpi_handle sem) {
  ((Semaphore *)sem)->cnt = 0;
  ((Semaphore *)sem)->invalid = 0;
  memset(&((Semaphore *)sem)->LOCK, 0, sizeof(Spinlock));
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type    type,
                                        uacpi_work_handler handler,
                                        uacpi_handle       ctx) {
  debugf("schedule work! todo!\n");
  panic();
  return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
  debugf("schedule work (wait)! todo!\n");
  panic();
  return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
  switch (req->type) {
  case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
    break;
  case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
    debugf("[acpi::glue] FATAL! Firmware error: type{%d} code{%d} arg{%d}\n",
           req->fatal.type, req->fatal.code, req->fatal.arg);
    break;
  }

  return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
  return (void *)(bootloader.hhdmOffset + addr);
}
void uacpi_kernel_unmap(void *addr, uacpi_size len) { (void)0; }

void uacpi_kernel_stall(uacpi_u8 usec) {
  size_t start = uacpi_kernel_get_nanoseconds_since_boot();
  if (!start)
    return; // timer initiated yet!
  size_t target = usec * 1000;
  while (uacpi_kernel_get_nanoseconds_since_boot() < (start + target))
    handControl();
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
  size_t start = timerTicks;
  if (!start)
    return; // timer initiated yet!
  while (timerTicks < (start + msec))
    handControl();
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address,
                                          uacpi_handle     *out_handle) {
  uacpi_pci_address *addr =
      (uacpi_pci_address *)malloc(sizeof(uacpi_pci_address));
  memcpy(addr, &address, sizeof(uacpi_pci_address));
  *out_handle = addr;
  return UACPI_STATUS_OK;
}
void uacpi_kernel_pci_device_close(uacpi_handle handle) { free(handle); }

uacpi_status uacpi_kernel_pci_read(uacpi_handle handle, uacpi_size offset,
                                   uacpi_u8 width, uacpi_u64 *out) {
  uacpi_pci_address *address = (uacpi_pci_address *)handle;
  if (address->segment != 0) {
    debugf("[acpi::glue] (write) Bad PCI segment{%d}!\n", address->segment);
    panic();
  }

  switch (width) {
  case 1:
    *out = ConfigReadWord(address->bus, address->device, address->function,
                          offset & ~1);
    *out = (*out >> ((offset & 1) * 8)) & 0xFF;
    break;
  case 2:
    *out = ConfigReadWord(address->bus, address->device, address->function,
                          offset);
    break;
  case 4:
    *out = ConfigReadWord(address->bus, address->device, address->function,
                          offset);
    *out |= ((uacpi_u64)ConfigReadWord(address->bus, address->device,
                                       address->function, offset + 2)
             << 16);
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }

  return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset,
                                    uacpi_u8 *value) {
  return uacpi_kernel_pci_read(device, offset, 1, (uacpi_u64 *)value);
}
uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset,
                                     uacpi_u16 *value) {
  return uacpi_kernel_pci_read(device, offset, 2, (uacpi_u64 *)value);
}
uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset,
                                     uacpi_u32 *value) {
  return uacpi_kernel_pci_read(device, offset, 4, (uacpi_u64 *)value);
}

uacpi_status uacpi_kernel_pci_write(uacpi_handle handle, uacpi_size offset,
                                    uacpi_u8 width, uacpi_u64 value) {
  uacpi_pci_address *address = (uacpi_pci_address *)handle;
  if (address->segment != 0) {
    debugf("[acpi::glue] Bad PCI segment{%d}!\n", address->segment);
    panic();
  }

  switch (width) {
  case 1: {
    {
      uint32_t dword = ConfigReadWord(address->bus, address->device,
                                      address->function, offset & ~0x3);
      uint32_t shift = (offset & 0x3) * 8;
      dword &= ~(0xFF << shift);
      dword |= (value & 0xFF) << shift;
      ConfigWriteDword(address->bus, address->device, address->function,
                       offset & ~0x3, dword);
    }
  } break;
  case 2: {
    uint32_t dword = ConfigReadWord(address->bus, address->device,
                                    address->function, offset & ~0x3);
    uint32_t shift = (offset & 0x2) * 8;
    dword &= ~(0xFFFF << shift);
    dword |= (value & 0xFFFF) << shift;
    ConfigWriteDword(address->bus, address->device, address->function,
                     offset & ~0x3, dword);
    break;
  }
  case 4:
    ConfigWriteDword(address->bus, address->device, address->function, offset,
                     value);
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }

  return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset,
                                     uacpi_u8 value) {
  return uacpi_kernel_pci_write(device, offset, 1, value);
}
uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset,
                                      uacpi_u16 value) {
  return uacpi_kernel_pci_write(device, offset, 2, value);
}
uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset,
                                      uacpi_u32 value) {
  return uacpi_kernel_pci_write(device, offset, 4, value);
}

// https://github.com/osdev0/cc-runtime/blob/dcdf5d82973e77edee597a047a3ef66300903de9/cc-runtime.c#L2229
int __popcountdi2(int64_t a) {
  uint64_t x2 = (uint64_t)a;
  x2 = x2 - ((x2 >> 1) & 0x5555555555555555uLL);
  // Every 2 bits holds the sum of every pair of bits (32)
  x2 = ((x2 >> 2) & 0x3333333333333333uLL) + (x2 & 0x3333333333333333uLL);
  // Every 4 bits holds the sum of every 4-set of bits (3 significant bits)
  // (16)
  x2 = (x2 + (x2 >> 4)) & 0x0F0F0F0F0F0F0F0FuLL;
  // Every 8 bits holds the sum of every 8-set of bits (4 significant bits)
  // (8)
  uint32_t x = (uint32_t)(x2 + (x2 >> 32));
  // The lower 32 bits hold four 16 bit sums (5 significant bits).
  //   Upper 32 bits are garbage
  x = x + (x >> 16);
  // The lower 16 bits hold two 32 bit sums (6 significant bits).
  //   Upper 16 bits are garbage
  return (x + (x >> 8)) & 0x0000007F; // (7 significant bits)
}
