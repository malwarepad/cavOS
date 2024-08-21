#include <elf.h>
#include <paging.h>
#include <stack.h>
#include <string.h>
#include <system.h>
#include <util.h>

// Stack creation for userland & kernelspace tasks
// Copyright (C) 2024 Panagiotis

void stackGenerateMutual(Task *task) {
  // if (GetPageDirectory() != task->pagedir) {
  //   debugf("[stack] Do NOT run stackGenerateMutual() without changing to the
  //   "
  //          "correct page directory first!\n");
  //   panic();
  // }

  // Map the user stack (for variables & such)
  for (int i = 0; i < USER_STACK_PAGES; i++) {
    size_t virt_addr =
        USER_STACK_BOTTOM - USER_STACK_PAGES * 0x1000 + i * 0x1000;
    VirtualMap(virt_addr, BitmapAllocatePageframe(&physical), PF_USER | PF_RW);
    memset((void *)virt_addr, 0, PAGE_SIZE);
  }
}

typedef struct StackStorePtrStyle {
  uint8_t *start;
  int      ellapsed;
} StackStorePtrStyle;
StackStorePtrStyle stackStorePtrStyle(Task *target, int ptrc, char **ptrv) {
  // Store argument contents
  uint32_t argSpace = 0;
  for (int i = 0; i < ptrc; i++)
    argSpace += strlength(ptrv[i]) + 1; // null terminator
  uint8_t *argStart = (uint8_t *)target->heap_end;
  taskAdjustHeap(target, target->heap_end + argSpace, &target->heap_start,
                 &target->heap_end);
  size_t ellapsed = 0;
  for (int i = 0; i < ptrc; i++) {
    uint32_t len = strlength(ptrv[i]) + 1; // null terminator
    memcpy((void *)((size_t)argStart + ellapsed), ptrv[i], len);
    ellapsed += len;
  }
  StackStorePtrStyle ret = {.start = argStart, .ellapsed = ellapsed};
  return ret;
}

void stackGenerateUser(Task *target, uint32_t argc, char **argv, uint32_t envc,
                       char **envv, uint8_t *out, size_t filesize,
                       void *elf_ehdr_ptr) {
  Elf64_Ehdr *elf_ehdr = (Elf64_Ehdr *)(elf_ehdr_ptr);

  // yeah, we will need to construct a stackframe...
  void *oldPagedir = GetPageDirectory();
  ChangePageDirectory(target->pagedir);

  stackGenerateMutual(target);

#define PUSH_TO_STACK(a, b, c)                                                 \
  a -= sizeof(b);                                                              \
  *((b *)(a)) = c

  int *randomByteStart = (int *)target->heap_end;
  taskAdjustHeap(target, target->heap_end + sizeof(int) * 4,
                 &target->heap_start, &target->heap_end);
  for (int i = 0; i < 4; i++) {
    int thing = 0;
    while (!thing)
      thing = rand();
    randomByteStart[i] = thing;
  }

  size_t lowestThing = 0;
  for (int i = 0; i < elf_ehdr->e_phnum; i++) {
    Elf64_Phdr *elf_phdr = (Elf64_Phdr *)((size_t)out + elf_ehdr->e_phoff +
                                          i * elf_ehdr->e_phentsize);
    if (elf_phdr->p_type != PT_LOAD)
      continue;
    if (!lowestThing || lowestThing > elf_phdr->p_vaddr)
      lowestThing = elf_phdr->p_vaddr;
  }

  // aux: AT_NULL
  PUSH_TO_STACK(target->registers.usermode_rsp, size_t, (size_t)0);
  PUSH_TO_STACK(target->registers.usermode_rsp, size_t, (size_t)0);
  // aux: AT_RANDOM
  PUSH_TO_STACK(target->registers.usermode_rsp, size_t,
                (size_t)randomByteStart);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 25);
  // aux: AT_PAGESZ
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 4096);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 6);
  // aux: AT_SECURE
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 23);
  // aux: AT_PHNUM
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, elf_ehdr->e_phnum);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 5);
  // aux: AT_PHENT
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t,
                elf_ehdr->e_phentsize);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 4);
  // aux: AT_ENTRY
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, elf_ehdr->e_entry);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 9);
  // aux: AT_BASE // todo: not hardcode
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0x100000000000);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 7);
  // aux: AT_FLAGS
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 8);
  // aux: AT_HWCAP
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 16);
  // aux: AT_PHDR
  PUSH_TO_STACK(target->registers.usermode_rsp, size_t,
                lowestThing + elf_ehdr->e_phoff);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 3);

  // store arguments & environment variables in heap
  StackStorePtrStyle arguments = stackStorePtrStyle(target, argc, argv);
  StackStorePtrStyle environment = {0};
  if (envc > 0)
    environment = stackStorePtrStyle(target, envc, envv);

  // end of environ
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0);
  if (environment.ellapsed > 0) {
    size_t finalEllapsed = 0;
    // ellapsed already has the full size lol
    for (int i = envc - 1; i >= 0; i--) {
      target->registers.usermode_rsp -= sizeof(uint64_t);
      uint64_t *finalEnvv = (uint64_t *)target->registers.usermode_rsp;

      uint32_t len = strlength(envv[i]) + 1; // null terminator
      finalEllapsed += len;
      *finalEnvv =
          (size_t)environment.start + (environment.ellapsed - finalEllapsed);
    }
  }

  // end of argv
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 0);

  // Store argument pointers (directly in stack)
  size_t finalEllapsed = 0;
  // ellapsed already has the full size lol
  for (int i = argc - 1; i >= 0; i--) {
    target->registers.usermode_rsp -= sizeof(uint64_t);
    uint64_t *finalArgv = (uint64_t *)target->registers.usermode_rsp;

    uint32_t len = strlength(argv[i]) + 1; // null terminator
    finalEllapsed += len;
    *finalArgv = (size_t)arguments.start + (arguments.ellapsed - finalEllapsed);
  }

  // argc
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, argc);

  ChangePageDirectory(oldPagedir);
}

void taskKernelReturn() {
  taskKill(currentTask->id, 0);
  while (1) {
  }
}

void stackGenerateKernel(Task *target, uint64_t parameter) {
  void *oldPagedir = GetPageDirectory();
  ChangePageDirectory(target->pagedir);

  stackGenerateMutual(target);
  target->registers.rdi = parameter;

  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t,
                (uint64_t)taskKernelReturn);

  ChangePageDirectory(oldPagedir);
}
