#include "task.h"
#include "types.h"

#ifndef STACK_H
#define STACK_H

void stackGenerateUser(Task *target, uint32_t argc, char **argv, uint32_t envc,
                       char **envv, uint8_t *out, size_t filesize,
                       void *elf_ehdr_ptr, size_t at_base);
void stackGenerateKernel(Task *target, uint64_t parameter);

#endif