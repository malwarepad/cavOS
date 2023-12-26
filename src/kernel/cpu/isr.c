#include <isr.h>
#include <paging.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>

// ISR Entry configurator
// Copyright (C) 2023 Panagiotis

char *format = "Kernel panic: %s!\n";

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

void handle_interrupt(AsmPassedInterrupt regs) {
  // if (regs.interrupt != 32 && regs.interrupt != 33 && regs.interrupt != 46)
  //   debugf("int %d!\n", regs.interrupt);
  if (regs.interrupt >= 32 && regs.interrupt <= 47) { // IRQ
    if (regs.interrupt >= 40) {
      outportb(0xA0, 0x20);
    }
    outportb(0x20, 0x20);

    switch (regs.interrupt) {
    case 32:
      timerTick();
      break;

    default:
      break;
    }
  } else if (regs.interrupt >= 0 && regs.interrupt <= 31) { // ISR
    // if (regs.interrupt == 14) {
    //   unsigned int err_pos;
    //   asm volatile("mov %%cr2, %0" : "=r"(err_pos));
    //   debugf("Page fault occured at: %x\n", err_pos);
    // }
    debugf(format, exceptions[regs.interrupt]);
    // if (framebuffer == KERNEL_GFX)
    //   printf(format, exceptions[regs.interrupt]);
    panic();
  } else if (regs.interrupt == 0x80) {
    // debugf("syscall %d!\n", regs.eax);
    syscallHandler(&regs);
  }
}
