#include "../../include/isr.h"

// ISR Entry configurator
// Copyright (C) 2023 Panagiotis

string format = "Kernel panic: %s!\n";

string exceptions[] = {"Division By Zero",
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

void isr0() {
  printf(format, exceptions[0]);
  asm("hlt");
}
void isr1() {
  printf(format, exceptions[1]);
  asm("hlt");
}
void isr2() {
  printf(format, exceptions[2]);
  asm("hlt");
}
void isr3() {
  printf(format, exceptions[3]);
  asm("hlt");
}
void isr4() {
  printf(format, exceptions[4]);
  asm("hlt");
}
void isr5() {
  printf(format, exceptions[5]);
  asm("hlt");
}
void isr6() {
  printf(format, exceptions[6]);
  asm("hlt");
}
void isr7() {
  printf(format, exceptions[7]);
  asm("hlt");
}
void isr8() {
  printf(format, exceptions[8]);
  asm("hlt");
}
void isr9() {
  printf(format, exceptions[9]);
  asm("hlt");
}
void isr10() {
  printf(format, exceptions[10]);
  asm("hlt");
}
void isr11() {
  printf(format, exceptions[11]);
  asm("hlt");
}
void isr12() {
  printf(format, exceptions[12]);
  asm("hlt");
}
void isr13() {
  printf(format, exceptions[13]);
  asm("hlt");
}
void isr14() {
  printf(format, exceptions[14]);
  asm("hlt");
}
void isr15() {
  printf(format, exceptions[15]);
  asm("hlt");
}
void isr16() {
  printf(format, exceptions[16]);
  asm("hlt");
}
void isr17() {
  printf(format, exceptions[17]);
  asm("hlt");
}
void isr18() {
  printf(format, exceptions[18]);
  asm("hlt");
}
void isr19() {
  printf(format, exceptions[19]);
  asm("hlt");
}
void isr20() {
  printf(format, exceptions[20]);
  asm("hlt");
}
void isr21() {
  printf(format, exceptions[21]);
  asm("hlt");
}
void isr22() {
  printf(format, exceptions[22]);
  asm("hlt");
}
void isr23() {
  printf(format, exceptions[23]);
  asm("hlt");
}
void isr24() {
  printf(format, exceptions[24]);
  asm("hlt");
}
void isr25() {
  printf(format, exceptions[25]);
  asm("hlt");
}
void isr26() {
  printf(format, exceptions[26]);
  asm("hlt");
}
void isr27() {
  printf(format, exceptions[27]);
  asm("hlt");
}
void isr28() {
  printf(format, exceptions[28]);
  asm("hlt");
}
void isr29() {
  printf(format, exceptions[29]);
  asm("hlt");
}
void isr30() {
  printf(format, exceptions[30]);
  asm("hlt");
}
void isr31() {
  printf(format, exceptions[31]);
  asm("hlt");
}

void isr_install() {
  // IRQs 0 - 15 -> 32 - 48
  remap_pic();

  // Exceptions & reserved -> 0 - 31
  set_idt_gate(0, (uint32)isr0);
  set_idt_gate(1, (uint32)isr1);
  set_idt_gate(2, (uint32)isr2);
  set_idt_gate(3, (uint32)isr3);
  set_idt_gate(4, (uint32)isr4);
  set_idt_gate(5, (uint32)isr5);
  set_idt_gate(6, (uint32)isr6);
  set_idt_gate(7, (uint32)isr7);
  set_idt_gate(8, (uint32)isr8);
  set_idt_gate(9, (uint32)isr9);
  set_idt_gate(10, (uint32)isr10);
  set_idt_gate(11, (uint32)isr11);
  set_idt_gate(12, (uint32)isr12);
  set_idt_gate(13, (uint32)isr13);
  set_idt_gate(14, (uint32)isr14);
  set_idt_gate(15, (uint32)isr15);
  set_idt_gate(16, (uint32)isr16);
  set_idt_gate(17, (uint32)isr17);
  set_idt_gate(18, (uint32)isr18);
  set_idt_gate(19, (uint32)isr19);
  set_idt_gate(20, (uint32)isr20);
  set_idt_gate(21, (uint32)isr21);
  set_idt_gate(22, (uint32)isr22);
  set_idt_gate(23, (uint32)isr23);
  set_idt_gate(24, (uint32)isr24);
  set_idt_gate(25, (uint32)isr25);
  set_idt_gate(26, (uint32)isr26);
  set_idt_gate(27, (uint32)isr27);
  set_idt_gate(28, (uint32)isr28);
  set_idt_gate(29, (uint32)isr29);
  set_idt_gate(30, (uint32)isr30);
  set_idt_gate(31, (uint32)isr31);

  // IRQs 0 - 15
  irq_install();

  // Finalize
  set_idt();
  asm("sti");
}