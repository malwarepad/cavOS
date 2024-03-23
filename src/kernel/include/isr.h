#include "types.h"

#ifndef ISR_H
#define ISR_H

#define MSRID_FSBASE 0xC0000100
#define MSRID_GSBASE 0xC0000101

#define MSRID_EFER 0xC0000080
#define MSRID_STAR 0xC0000081
#define MSRID_LSTAR 0xC0000082
#define MSRID_FMASK 0xC0000084

#define RFLAGS_IF (1 << 9)
#define RFLAGS_DF (1 << 10)

typedef struct {
  uint64_t ds; // & es

  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rbp;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;

  uint64_t interrupt;
  uint64_t error;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t usermode_rsp;
  uint64_t usermode_ss;
} AsmPassedInterrupt;

void initiateISR();
void registerIRQhandler(uint8_t id, void *handler);

extern void  asm_isr_exit();
extern void *asm_isr_redirect_table[];
extern void  isr128();

#endif
