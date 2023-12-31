#include "types.h"

#ifndef ISR_H
#define ISR_H

typedef struct {
  // pushed by us:
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // esp is ignored
  uint32_t interrupt, error;

  // pushed by the CPU:
  uint32_t eip, cs, eflags, usermode_esp, usermode_ss;
} AsmPassedInterrupt;

void isr_install();
void handle_interrupt(AsmPassedInterrupt regs);
void registerIRQhandler(uint8_t id, void *handler);

void  asm_isr_exit();
void *asm_isr_redirect_table[];
void  isr128();

#endif
