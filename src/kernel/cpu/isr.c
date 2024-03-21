#include <idt.h>
#include <isr.h>
#include <kb.h>
#include <linked_list.h>
#include <nic_controller.h>
#include <paging.h>
#include <rtl8139.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

// ISR Entry configurator
// Copyright (C) 2024 Panagiotis

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
  irqHandler *next;

  uint8_t      id;
  FunctionPtr *handler;
};
irqHandler *firstIrqHandler = 0;

void initiateISR() {
  // IRQs 0 - 15 -> 32 - 48
  remap_pic();

  // ISR exceptions 0 - 31
  for (int i = 0; i < 48; i++) {
    set_idt_gate(i, (uint64_t)asm_isr_redirect_table[i], 0x8E);
  }

  // Syscalls having DPL 3
  set_idt_gate(0x80, (uint64_t)isr128, 0xEE);

  // Finalize
  set_idt();
  asm volatile("sti");
}

void registerIRQhandler(uint8_t id, void *handler) {
  // printf("IRQ %d reserved!\n", id);
  irqHandler *target =
      (irqHandler *)LinkedListAllocate(&firstIrqHandler, sizeof(irqHandler));

  target->id = id;
  target->handler = handler;
  target->next = 0; // null ptr
}

void handleTaskFault(AsmPassedInterrupt *regs) {
  debugf("[isr::task] Killing task{%d} because of %s!\n", currentTask->id,
         exceptions[regs->interrupt]);
  kill_task(currentTask->id);
}

// pass stack ptr
void handle_interrupt(uint64_t rsp) {
  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;
  if (cpu->interrupt >= 32 && cpu->interrupt <= 47) { // IRQ
    if (cpu->interrupt >= 40) {
      outportb(0xA0, 0x20);
    }
    outportb(0x20, 0x20);
    switch (cpu->interrupt) {
    case 32 + 0: // irq0
      timerTick(cpu);
      break;

    case 32 + 1: // irq1
      kbIrq();
      break;

    default: // execute other handlers
      irqHandler *browse = firstIrqHandler;
      while (browse) {
        if (browse->id == (cpu->interrupt - 32)) {
          FunctionPtr handler = browse->handler;
          handler(&cpu);
        }
        browse = browse->next;
      }
      break;
    }
  } else if (cpu->interrupt >= 0 && cpu->interrupt <= 31) { // ISR
    if (systemCallOnProgress)
      debugf("[isr] Happened from system call!\n");

    if (!systemCallOnProgress && tasksInitiated &&
        currentTask->id != KERNEL_TASK && !currentTask->kernel_task) {
      handleTaskFault(&cpu);
      return;
    }

    if (cpu->interrupt == 14) {
      uint64_t err_pos;
      asm volatile("movq %%cr2, %0" : "=r"(err_pos));
      debugf("[isr] Page fault occured at: %lx\n", err_pos);
    }
    debugf(format, exceptions[cpu->interrupt]);
    // if (framebuffer == KERNEL_GFX)
    //   printf(format, exceptions[cpu->interrupt]);
    panic();
  } else if (cpu->interrupt == 0x80) {
    systemCallOnProgress = true;
    syscallHandler(cpu);
    systemCallOnProgress = false;
  }
}
