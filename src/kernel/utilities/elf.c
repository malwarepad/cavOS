#include <bitmap.h>
#include <elf.h>
#include <fs_controller.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <system.h>
#include <task.h>
#include <util.h>

// ELF (for now only 64) parser
// Copyright (C) 2024 Panagiotis

#define ELF_DEBUG 0

bool elf_check_file(Elf64_Ehdr *hdr) {
  if (!hdr)
    return false;
  if (hdr->e_ident[EI_MAG0] != ELFMAG0) {
    debugf("[elf] Header EI_MAG0 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG1] != ELFMAG1) {
    debugf("[elf] Header EI_MAG1 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG2] != ELFMAG2) {
    debugf("[elf] Header EI_MAG2 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG3] != ELFMAG3) {
    debugf("[elf] Header EI_MAG3 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      hdr->e_machine != ELF_x86_64_MACHINE) {
    debugf("[elf] Architecture is not supported.\n");
    return false;
  }
  return true;
}

uint32_t elf_execute(char *filepath, uint32_t argc, char **argv) {
  lockInterrupts();

  // Open & read executable file
  OpenFile *dir = fsKernelOpen(filepath);
  if (!dir) {
    debugf("[elf] Could not open %s\n", filepath);
    releaseInterrupts();
    return 0;
  }
#if ELF_DEBUG
  debugf("[elf] Executing %s: filesize{%d}\n", filepath, dir->filesize);
#endif
  uint8_t *out = (uint8_t *)malloc(fsGetFilesize(dir));
  fsReadFullFile(dir, out);
  fsKernelClose(dir);

  // Cast ELF32 header
  Elf64_Ehdr *elf_ehdr = (Elf64_Ehdr *)(out);

  if (!elf_check_file(elf_ehdr)) {
    debugf("[elf] File %s is not a valid cavOS ELF32 executable!\n", filepath);
    releaseInterrupts();
    return 0;
  }

  // Create a new page directory which is later used by the process
  uint64_t *oldpagedir = GetPageDirectory();
  uint64_t *pagedir = PageDirectoryAllocate();
  ChangePageDirectory(pagedir);

#if ELF_DEBUG
  debugf("\n[elf_ehdr] entry=%x type=%d arch=%d\n", elf_ehdr->e_entry,
         elf_ehdr->e_type, elf_ehdr->e_machine);
  debugf("[early phdr] offset=%x count=%d size=%d\n", elf_ehdr->e_phoff,
         elf_ehdr->e_phnum, elf_ehdr->e_phentsize);
#endif

  int32_t id = create_taskid();
  if (id == -1) {
    debugf(
        "[elf] Cannot fetch task id... You probably reached the task limit!");
    // optional
    // printf("[elf] Cannot fetch task id... You probably reached the task
    // limit!");
    panic();
  }

  // Loop through the multiple ELF32 program header tables
  for (int i = 0; i < elf_ehdr->e_phnum; i++) {
    Elf64_Phdr *elf_phdr = (Elf64_Phdr *)((size_t)out + elf_ehdr->e_phoff +
                                          i * elf_ehdr->e_phentsize);
    if (elf_phdr->p_type != PT_LOAD)
      continue;

    // Map the (current) program page
    uint64_t pagesRequired = DivRoundUp(elf_phdr->p_memsz, 0x1000);
    for (int j = 0; j < pagesRequired; j++) {
      size_t vaddr = (elf_phdr->p_vaddr & ~0xFFF) + j * 0x1000;
      size_t paddr = BitmapAllocatePageframe(&physical);
      VirtualMap(vaddr, paddr, PF_SYSTEM | PF_USER | PF_RW);
    }

    // Copy the required info
    memcpy(elf_phdr->p_vaddr, out + elf_phdr->p_offset, elf_phdr->p_filesz);

    uint64_t file_start = (elf_phdr->p_vaddr & ~0xFFF) + elf_phdr->p_filesz;
    uint64_t file_end = (elf_phdr->p_vaddr & ~0xFFF) + pagesRequired * 0x1000;
    memset(file_start, 0, file_end - file_start);

#if ELF_DEBUG
    debugf("[elf] Program header: type{%d} offset{%x} vaddr{%x} size{%x} "
           "alignment{%x}\n",
           elf_phdr->p_type, elf_phdr->p_offset, elf_phdr->p_vaddr,
           elf_phdr->p_memsz, elf_phdr->p_align);
#endif
  }

  // For the foreseeable future ;)
#if ELF_DEBUG
  for (int i = 0; i < elf_ehdr->e_shnum; i++) {
    Elf32_Shdr *elf_shdr = (Elf32_Shdr *)((uint32_t)out + elf_ehdr->e_shoff +
                                          i * elf_ehdr->e_shentsize);
    debugf("[elf] Section header: type{%d} offset{%x}\n", elf_shdr->sh_type,
           elf_shdr->sh_offset);
  }
#endif

  // Map the user stack (for variables & such)
  for (int i = 0; i < USER_STACK_PAGES; i++) {
    VirtualMap(USER_STACK_BOTTOM - USER_STACK_PAGES * 0x1000 + i * 0x1000,
               BitmapAllocatePageframe(&physical), PF_SYSTEM | PF_USER | PF_RW);
  }

#if ELF_DEBUG
  debugf("[elf] New pagedir: offset{%x}\n", pagedir);
#endif

  Task *target =
      create_task(id, (uint64_t)elf_ehdr->e_entry, false, pagedir, argc, argv);

  // yeah, we will need to construct a stackframe...
  void *oldPagedir = GetPageDirectory();
  ChangePageDirectory(target->pagedir);

#define PUSH_TO_STACK(a, b, c)                                                 \
  a -= sizeof(b);                                                              \
  *((b *)(a)) = c

  int *randomByteStart = target->heap_end;
  adjust_user_heap(target, target->heap_end + 16);
  for (int i = 0; i < 4; i++)
    randomByteStart[i] = rand();

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
  // aux: AT_PHNUM
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, elf_ehdr->e_phnum);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 5);
  // aux: AT_PHENT
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t,
                elf_ehdr->e_shentsize);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 4);
  // aux: AT_PHDR
  PUSH_TO_STACK(target->registers.usermode_rsp, size_t,
                (size_t)out + elf_ehdr->e_phoff);
  PUSH_TO_STACK(target->registers.usermode_rsp, uint64_t, 3);

  // Store argument contents
  uint32_t argSpace = 0;
  for (int i = 0; i < argc; i++)
    argSpace += strlength(argv[i]) + 1; // null terminator
  uint8_t *argStart = target->heap_end;
  adjust_user_heap(target, target->heap_end + argSpace);
  size_t ellapsed = 0;
  for (int i = 0; i < argc; i++) {
    uint32_t len = strlength(argv[i]) + 1; // null terminator
    memcpy((size_t)argStart + ellapsed, argv[i], len);
    ellapsed += len;
  }

  // todo: Proper environ
  uint64_t *environStart = target->heap_end;
  adjust_user_heap(target, target->heap_end + sizeof(uint64_t) * 10);
  memset(environStart, 0, sizeof(uint64_t) * 10);
  environStart[0] = &environStart[5];

  // Reserve stack space for environ
  target->registers.usermode_rsp -= sizeof(uint64_t);
  uint64_t *finalEnviron = target->registers.usermode_rsp;

  // Store argument pointers (directly in stack)
  size_t finalEllapsed = 0;
  // ellapsed already has the full size lol
  for (int i = argc - 1; i >= 0; i--) {
    target->registers.usermode_rsp -= sizeof(uint64_t);
    uint64_t *finalArgv = target->registers.usermode_rsp;

    uint32_t len = strlength(argv[i]) + 1; // null terminator
    finalEllapsed += len;
    *finalArgv = (size_t)argStart + (ellapsed - finalEllapsed);
  }

  // Reserve stack space for argc
  target->registers.usermode_rsp -= sizeof(uint64_t);
  uint64_t *finalArgc = target->registers.usermode_rsp;

  // Put everything left in the stack, as expected
  *finalArgc = argc;
  *finalEnviron = finalEnviron;

  ChangePageDirectory(oldPagedir);

  // Cleanup...
  free(out);
  ChangePageDirectory(oldpagedir);

  releaseInterrupts();

  return id;
}
