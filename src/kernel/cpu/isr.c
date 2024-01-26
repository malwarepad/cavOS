#include <isr.h>
#include <nic_controller.h>
#include <paging.h>
#include <rtl8139.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>

// ISR Entry configurator
// Copyright (C) 2023 Panagiotis

char *format = "Kernel panic: %s!\n";

char *exceptions[] = {"[isr] Division By Zero",
                      "[isr] Debug",
                      "[isr] Non Maskable Interrupt",
                      "[isr] Breakpoint",
                      "[isr] Into Detected Overflow",
                      "[isr] Out of Bounds",
                      "[isr] Invalid Opcode",
                      "[isr] No Coprocessor",

                      "[isr] Double Fault",
                      "[isr] Coprocessor Segment Overrun",
                      "[isr] Bad TSS",
                      "[isr] Segment Not Present",
                      "[isr] Stack Fault",
                      "[isr] General Protection Fault",
                      "[isr] Page Fault",
                      "[isr] Unknown Interrupt",

                      "[isr] Coprocessor Fault",
                      "[isr] Alignment Check",
                      "[isr] Machine Check",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",

                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved",
                      "[isr] Reserved"};

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
FunctionPtr irqHandlers[16]; // IRQs 0 - 15

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
  irqHandlers[id] = handler;
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

    default: // execute other handlers
      if (irqHandlers[regs.interrupt - 32])
        irqHandlers[regs.interrupt - 32](&regs);
      break;
    }
  } else if (regs.interrupt >= 0 && regs.interrupt <= 31) { // ISR
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
    syscallHandler(&regs);
  }
}
