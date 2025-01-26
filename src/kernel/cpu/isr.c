#include <apic.h>
#include <idt.h>
#include <isr.h>
#include <kb.h>
#include <linked_list.h>
#include <nic_controller.h>
#include <paging.h>
#include <rtl8139.h>
#include <schedule.h>
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

void disable_pic() {
  // outportb(0x20, ICW_1);
  // outportb(0xA0, ICW_1);
  // outportb(0x21, ICW_2_M);
  // outportb(0xA1, ICW_2_S);
  // outportb(0x21, ICW_3_M);
  // outportb(0xA1, ICW_3_S);
  // outportb(0x21, ICW_4);
  // outportb(0xA1, ICW_4);
  outportb(0x21, 0xFF);
  outportb(0xA1, 0xFF);
}

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

  disable_pic();
}

irqHandler *firstIrqHandler = 0;

extern void isr255();
void        initiateISR() {
  // IRQs 0 - 15 -> 32 - 48
  remap_pic();

  // ISR exceptions 0 - 31
  for (int i = 0; i < 48; i++) {
    set_idt_gate(i, (uint64_t)asm_isr_redirect_table[i], 0x8E);
  }

  // APIC Spurious Interrupts
  set_idt_gate(0xff, (uint64_t)isr255, 0x8E);

  // Syscalls having DPL 3
  set_idt_gate(0x80, (uint64_t)isr128, 0xEE);

  // Finalize
  set_idt();
  initiateAPIC();
  asm volatile("sti");
}

irqHandler *registerIRQhandler(uint8_t id, void *handler) {
  // printf("IRQ %d reserved!\n", id);
  irqHandler *target = (irqHandler *)LinkedListAllocate(
      (void **)(&firstIrqHandler), sizeof(irqHandler));

  target->id = id;
  target->handler = handler;
  // target->next = 0; // null ptr

  return target;
}

void handleTaskFault(AsmPassedInterrupt *regs) {
  if (regs->interrupt == 14) {
    uint64_t err_pos;
    asm volatile("movq %%cr2, %0" : "=r"(err_pos));
    debugf("[isr] Page fault occured at cr2{%lx} rip{%lx}\n", err_pos,
           regs->rip);
  }
  debugf("[isr::task] [%c] Killing task{%d} because of %s!\n",
         currentTask->kernel_task ? '-' : 'u', currentTask->id,
         exceptions[regs->interrupt]);
  // debugf("at %lx\n", regs->rip);
  // panic();
  taskKill(currentTask->id, 139);
  schedule((uint64_t)regs);
}

uint64_t handle_syscall_tssrsp(uint64_t rsp) {
  if (!tasksInitiated)
    return rsp;

  void *cpu = (void *)rsp;

  void *iretqRsp =
      (void *)(currentTask->whileSyscallRsp - sizeof(AsmPassedInterrupt) - 8);
  memcpy(iretqRsp, cpu, sizeof(AsmPassedInterrupt) + 8);

  return (size_t)iretqRsp;
}

uint64_t handle_tssrsp(uint64_t rsp) {
  if (!tasksInitiated)
    return rsp;

  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;

  AsmPassedInterrupt *iretqRsp =
      (AsmPassedInterrupt *)(currentTask->whileTssRsp -
                             sizeof(AsmPassedInterrupt));
  memcpy(iretqRsp, cpu, sizeof(AsmPassedInterrupt));

  return (size_t)iretqRsp;
}

// pass stack ptr
void handle_interrupt(uint64_t rsp) {
  AsmPassedInterrupt *cpu = (AsmPassedInterrupt *)rsp;
  if (cpu->interrupt >= 32 && cpu->interrupt <= 47) { // IRQ
    /* Ack the IRQ respectively */
    if (cpu->interrupt >= 40) {
      outportb(0xA0, 0x20);
    }
    outportb(0x20, 0x20);
    apicWrite(0xB0, 0);

    /* find handler */
    irqHandler *browse = firstIrqHandler;
    while (browse) {
      if (browse->id == cpu->interrupt) {
        FunctionPtr handler = browse->handler;
        handler(browse->argument ? (AsmPassedInterrupt *)browse->argument
                                 : cpu);
      }
      browse = browse->next;
    }
  } else if (cpu->interrupt >= 0 && cpu->interrupt <= 31) { // ISR
    // To drop the current execution and give control to the scheduler, set this
    // variable and generate a page fault onto the magic address
    if (currentTask->schedPageFault && cpu->interrupt == 14) {
      uint64_t errorLocation = 0;
      asm volatile("movq %%cr2, %0" : "=r"(errorLocation));
      if (errorLocation == SCHED_PAGE_FAULT_MAGIC_ADDRESS) {
        currentTask->schedPageFault = false;
        cpu->rip++;
        schedule((uint64_t)cpu);
        return;
      }
    }

    if (currentTask->systemCallInProgress)
      debugf("[isr] Happened from system call!\n");

    if (!currentTask->systemCallInProgress && tasksInitiated &&
        currentTask->id != KERNEL_TASK_ID) { // && !currentTask->kernel_task
      handleTaskFault(cpu);
      return;
    }

    if (cpu->interrupt == 14) {
      uint64_t err_pos;
      asm volatile("movq %%cr2, %0" : "=r"(err_pos));
      debugf("[isr] Page fault occured at cr2{%lx} rip{%lx}\n", err_pos,
             cpu->rip);
    }
    debugf(format, exceptions[cpu->interrupt]);
    // if (framebuffer == KERNEL_GFX)
    //   printf(format, exceptions[cpu->interrupt]);
    panic();
  } else if (cpu->interrupt == 0x80) {
    syscallHandler(cpu);
  }
}
