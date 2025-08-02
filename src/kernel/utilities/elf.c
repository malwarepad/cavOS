#include <bitmap.h>
#include <console.h>
#include <elf.h>
#include <fb.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <stack.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <util.h>
#include <vfs.h>
#include <vmm.h>

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

void elfProcessLoad(Elf64_Phdr *elf_phdr, uint8_t *out, size_t base) {
  // Map the (current) program page
  size_t   startRounded = (elf_phdr->p_vaddr & ~0xFFF);
  uint64_t pagesRequired = DivRoundUp(
      (elf_phdr->p_vaddr - startRounded) + elf_phdr->p_memsz, 0x1000);
  for (int j = 0; j < pagesRequired; j++) {
    size_t vaddr = (elf_phdr->p_vaddr & ~0xFFF) + j * 0x1000;
    if (VirtualToPhysical(base + vaddr))
      continue;
    size_t paddr = PhysicalAllocate(1);
    VirtualMap(base + vaddr, paddr, PF_USER | PF_RW);
  }

  // Copy the required info
  memcpy((void *)(base + elf_phdr->p_vaddr), out + elf_phdr->p_offset,
         elf_phdr->p_filesz);

  // wtf is this? (needed)
  if (elf_phdr->p_memsz > elf_phdr->p_filesz)
    memset((void *)(base + elf_phdr->p_vaddr + elf_phdr->p_filesz), 0,
           elf_phdr->p_memsz - elf_phdr->p_filesz);
}

Task *elfExecute(char *filepath, uint32_t argc, char **argv, uint32_t envc,
                 char **envv, bool startup) {
  // Open & read executable file
  OpenFile *dir = fsKernelOpen(filepath, O_RDONLY, 0);
  if (!dir) {
    debugf("[elf] Could not open %s\n", filepath);
    return 0;
  }
  size_t filesize = fsGetFilesize(dir);
#if ELF_DEBUG
  debugf("[elf] Executing %s: filesize{%d}\n", filepath, filesize);
#endif
  size_t   outSize = DivRoundUp(filesize, BLOCK_SIZE);
  uint8_t *out = (uint8_t *)VirtualAllocate(outSize);
  fsRead(dir, out, filesize);
  fsKernelClose(dir);

  // Cast ELF32 header
  Elf64_Ehdr *elf_ehdr = (Elf64_Ehdr *)(out);

  if (!elf_check_file(elf_ehdr)) {
    debugf("[elf] File %s is not a valid cavOS ELF32 executable!\n", filepath);
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

  int32_t id = taskGenerateId();
  if (id == -1) {
    debugf(
        "[elf] Cannot fetch task id... You probably reached the task limit!");
    // optional
    // printf("[elf] Cannot fetch task id... You probably reached the task
    // limit!");
    panic();
  }

  size_t interpreterEntry = 0;
  size_t interpreterBase = 0x100000000000; // todo: not hardcode
  // Loop through the multiple ELF32 program header tables
  for (int i = 0; i < elf_ehdr->e_phnum; i++) {
    Elf64_Phdr *elf_phdr = (Elf64_Phdr *)((size_t)out + elf_ehdr->e_phoff +
                                          i * elf_ehdr->e_phentsize);
    if (elf_phdr->p_type == 3) {
      char     *interpreterFilename = (char *)(out + elf_phdr->p_offset);
      OpenFile *interpreter = fsKernelOpen(interpreterFilename, O_RDONLY, 0);
      if (!interpreter) {
        debugf("[elf] Interpreter path{%s} could not be found!\n",
               interpreterFilename);
        panic();
      }
      size_t size = fsGetFilesize(interpreter);

      size_t   intContentsSize = DivRoundUp(size, BLOCK_SIZE);
      uint8_t *interpreterContents =
          (uint8_t *)VirtualAllocate(intContentsSize);
      fsRead(interpreter, interpreterContents, size);
      fsKernelClose(interpreter);

      Elf64_Ehdr *interpreterEhdr = (Elf64_Ehdr *)(interpreterContents);
      if (interpreterEhdr->e_type != 3) { // ET_DYN
        debugf("[elf::dyn] Interpreter{%s} isn't really of type ET_DYN!\n",
               interpreterFilename);
        panic();
      }
      interpreterEntry = interpreterEhdr->e_entry;
      for (int i = 0; i < interpreterEhdr->e_phnum; i++) {
        Elf64_Phdr *interpreterPhdr =
            (Elf64_Phdr *)((size_t)interpreterContents +
                           interpreterEhdr->e_phoff +
                           i * interpreterEhdr->e_phentsize);
        if (interpreterPhdr->p_type != PT_LOAD)
          continue;
        elfProcessLoad(interpreterPhdr, interpreterContents, interpreterBase);
      }
      VirtualFree(interpreterContents, intContentsSize);

      continue;
    }
    if (elf_phdr->p_type != PT_LOAD)
      continue;

    elfProcessLoad(elf_phdr, out, 0);

#if ELF_DEBUG
    debugf("[elf] Program header: type{%d} offset{%x} vaddr{%x} size{%x} "
           "alignment{%x}\n",
           elf_phdr->p_type, elf_phdr->p_offset, elf_phdr->p_vaddr,
           elf_phdr->p_memsz, elf_phdr->p_align);
#endif
  }

  // For the foreseeable future ;)
#if ELF_DEBUG
  // for (int i = 0; i < elf_ehdr->e_shnum; i++) {
  //   Elf64_Shdr *elf_shdr = (Elf64_Shdr *)((uint32_t)out + elf_ehdr->e_shoff +
  //                                         i * elf_ehdr->e_shentsize);
  //   debugf("[elf] Section header: type{%d} offset{%lx}\n", elf_shdr->sh_type,
  //          elf_shdr->sh_offset);
  // }
#endif

#if ELF_DEBUG
  debugf("[elf] New pagedir: offset{%x}\n", pagedir);
#endif

  // Done loading into the pagedir
  ChangePageDirectory(oldpagedir);

  Task *target =
      taskCreate(id,
                 interpreterEntry ? (interpreterBase + interpreterEntry)
                                  : elf_ehdr->e_entry,
                 false, pagedir, argc, argv);

  size_t totalLen = 0;
  for (int curr = 0; curr < argc; curr++)
    totalLen += strlength(argv[curr]) + 1;
  char  *cmdline = malloc(totalLen);
  size_t strCurr = 0;
  for (int curr = 0; curr < argc; curr++) {
    int f = strlength(argv[curr]) + 1;
    memcpy(&cmdline[strCurr], argv[curr], f);
    strCurr += f;
  }

  target->cmdline = cmdline;
  target->cmdlineLen = totalLen;
  target->execname = strdup(filepath);

  // libc takes care of tls lmao
  /*if (tls) {
    ChangePageDirectory(pagedir);

    size_t tls_start = tls->p_vaddr;
    size_t tls_end = tls->p_vaddr + tls->p_memsz;

    debugf("[elf::tls] Found: virt{%lx} len{%lx}\n", tls->p_vaddr,
           tls->p_memsz);
    uint8_t *tls = (uint8_t *)target->heap_end;
    taskAdjustHeap(target, target->heap_end + 4096);

    target->fsbase = (size_t)tls + 512;
    *(uint64_t *)(tls + 512) = (size_t)tls + 512;

    uint8_t *tlsp =
        (uint8_t *)((size_t)tls + 512 - (tls_end - tls_start)); // copy tls
    for (uint8_t *i = (uint8_t *)tls_start; (size_t)i < tls_end; i++)
      *tlsp++ = *i++;

    ChangePageDirectory(oldpagedir);
  }*/

  // Current working directory init (should be already done in task.c)
  // target->cwd = (char *)malloc(2);
  // target->cwd[0] = '/';
  // target->cwd[1] = '\0';

  // User stack generation: the stack itself, AUXs, etc...
  stackGenerateUser(target, argc, argv, envc, envv, out, filesize, elf_ehdr,
                    interpreterEntry ? 0x100000000000 : 0);
  VirtualFree(out, outSize);

  // Align it, just in case...
  spinlockAcquire(&target->infoPd->LOCK_PD);
  taskAdjustHeap(target, DivRoundUp(target->infoPd->heap_end, 0x1000) * 0x1000,
                 &target->infoPd->heap_start, &target->infoPd->heap_end);
  spinlockRelease(&target->infoPd->LOCK_PD);

  // Just a sane default
  target->parent = currentTask;

  if (startup)
    taskCreateFinish(target);

  return target;
}
