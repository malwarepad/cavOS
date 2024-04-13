#include "task.h"
#include "types.h"

#ifndef STACK_H
#define STACK_H

void stackGenerateUser(Task *target, uint32_t argc, char **argv, uint8_t *out,
                       size_t filesize, void *elf_ehdr_ptr);
void stackGenerateKernel(Task *target, uint64_t parameter);

#endif