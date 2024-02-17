#include <isr.h>
#include <kb.h>
#include <nic_controller.h>
#include <paging.h>
#include <rtl8139.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>

// ISR Entry configurator
// Copyright (C) 2023 Panagiotis

char *format = "[isr] Kernel panic: %s!\n";

char *exceptions[] = {"Division By Zero",
                      "Debug",
                      "Non Maskable Interrupt",
                      "Breakpoint",
                      "Into Detected Overflow",
                      "Out of Bounds",
                      "Invalid Opcode",
                      "No Coprocessor",

                      "Double Fault",
                      "Coprocessor Segment Overrun",
                      "Bad TSS",
                      "Segment Not Present",
                      "Stack Fault",
                      "General Protection Fault",
                      "Page Fault",
                      "Unknown Interrupt",

                      "Coprocessor Fault",
                      "Alignment Check",
                      "Machine Check",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",

                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved",
                      "Reserved"};

void remap_pic() {
  outportb(0x20, 0x11);
  outportb(0xA0, 0x11);
  outportb(0x21, 0x20);
  outportb(0xA1, 0x28);
  outportb(0x21, 0x04);
  outportb(0xA1, 0x02);
  outportb(0x21, 0x01);
  outportb(0xA1, 0x01);
  outportb(0x21, 0x00);
  outportb(0xA1, 0x00);
}

typedef void (*FunctionPtr)(AsmPassedInterrupt *regs);
// FunctionPtr irqHandlers[16]; // IRQs 0 - 15
typedef struct irqHandler irqHandler;
struct irqHandler {
  uint8_t      id;
  FunctionPtr *handler;

  irqHandler *next;
};
irqHandler *firstIrqHandler = 0;

void isr_install() {
  // IRQs 0 - 15 -> 32 - 48
  remap_pic();

  // ISR exceptions 0 - 31
  for (int i = 0; i < 48; i++) {
    set_idt_gate(i, asm_isr_redirect_table[i], 0x8E);
  }

  // Syscalls having DPL 3
  set_idt_gate(0x80, isr128, 0xEE);

  // Finalize
  set_idt();
  asm("sti");
}

void registerIRQhandler(uint8_t id, void *handler) {
  // printf("IRQ %d reserved!\n", id);
  irqHandler *target = (irqHandler *)malloc(sizeof(irqHandler));
  memset(target, 0, sizeof(irqHandler));
  irqHandler *curr = firstIrqHandler;
  while (1) {
    if (curr == 0) {
      // means this is our first one
      firstIrqHandler = target;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = target;
      break;
    }
    curr = curr->next; // cycle
  }

  target->id = id;
  target->handler = handler;
  target->next = 0; // null ptr
}

void handleTaskFault(AsmPassedInterrupt *regs) {
  debugf("[isr::task] Killing task{%d} because of %s!\n", currentTask->id,
         exceptions[regs->interrupt]);
  kill_task(currentTask->id);
}

void handle_interrupt(AsmPassedInterrupt regs) {
  if (regs.interrupt >= 32 && regs.interrupt <= 47) { // IRQ
    if (regs.interrupt >= 40) {
      outportb(0xA0, 0x20);
    }
    outportb(0x20, 0x20);
    switch (regs.interrupt) {
    case 32 + 0: // irq0
      timerTick();
      break;

    case 32 + 1: // irq1
      kbIrq();
      break;

    default: // execute other handlers
      irqHandler *browse = firstIrqHandler;
      while (browse) {
        if (browse->id == (regs.interrupt - 32)) {
          FunctionPtr handler = browse->handler;
          handler(&regs);
        }
        browse = browse->next;
      }
      break;
    }
  } else if (regs.interrupt >= 0 && regs.interrupt <= 31) { // ISR
    if (systemCallOnProgress)
      debugf("[isr] Happened from system call!\n");

    if (!systemCallOnProgress && currentTask->id != KERNEL_TASK &&
        !currentTask->kernel_task) {
      handleTaskFault(&regs);
      return;
    }

    if (regs.interrupt == 14) {
      unsigned int err_pos;
      asm volatile("mov %%cr2, %0" : "=r"(err_pos));
      debugf("[isr] Page fault occured at: %x\n", err_pos);
    }
    debugf(format, exceptions[regs.interrupt]);
    // if (framebuffer == KERNEL_GFX)
    //   printf(format, exceptions[regs.interrupt]);
    panic();
  } else if (regs.interrupt == 0x80) {
    systemCallOnProgress = true;
    syscallHandler(&regs);
    systemCallOnProgress = false;
  }
}
