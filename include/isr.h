#include "idt.h"
#include "irq.h"
#include "tty.h"
#include "types.h"
#include "util.h"

#ifndef ISR_H
#define ISR_H

void isr_install();

#endif
