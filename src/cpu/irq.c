#include "../../include/irq.h"

// IRQ Entry configurator
// Copyright (C) 2023 Panagiotis

// sti assembly call is temporary because of invalid
// function return code!
// todo: make this return correctly
// instead of re-enabling interrupts

// todo: use IRQs as much as possible especially for
// shift & caps lock.
void irq0() {
  // timerTick();
  outportb(0x20, 0x20);
  asm("sti");
}
void irq1() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq2() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq3() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq4() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq5() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq6() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq7() {
  outportb(0x20, 0x20);
  asm("sti");
}
void irq8() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq9() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq10() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq11() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq12() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq13() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq14() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}
void irq15() {
  outportb(0xA0, 0x20);
  outportb(0x20, 0x20);
  asm("sti");
}

void irq_install() {
  set_idt_gate(32, (uint32)irq0);
  set_idt_gate(33, (uint32)irq1);
  set_idt_gate(34, (uint32)irq2);
  set_idt_gate(35, (uint32)irq3);
  set_idt_gate(36, (uint32)irq4);
  set_idt_gate(37, (uint32)irq5);
  set_idt_gate(38, (uint32)irq6);
  set_idt_gate(39, (uint32)irq7);
  set_idt_gate(40, (uint32)irq8);
  set_idt_gate(41, (uint32)irq9);
  set_idt_gate(42, (uint32)irq10);
  set_idt_gate(43, (uint32)irq11);
  set_idt_gate(44, (uint32)irq12);
  set_idt_gate(45, (uint32)irq13);
  set_idt_gate(46, (uint32)irq14);
  set_idt_gate(47, (uint32)irq15);

  asm("sti");
}
