#include <bitmap.h>
#include <elf.h>
#include <fs_controller.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <stack.h>
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

uint32_t elfExecute(char *filepath, uint32_t argc, char **argv) {
  // Open & read executable file
  OpenFile *dir = fsKernelOpen(filepath, FS_MODE_READ, 0);
  if (!dir) {
    debugf("[elf] Could not open %s\n", filepath);
    return 0;
  }
  size_t filesize = fsGetFilesize(dir);
#if ELF_DEBUG
  debugf("[elf] Executing %s: filesize{%d}\n", filepath, filesize);
#endif
  uint8_t *out = (uint8_t *)malloc(filesize);
  fsReadFullFile(dir, out);
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

  size_t tls = 0;
  // Loop through the multiple ELF32 program header tables
  for (int i = 0; i < elf_ehdr->e_phnum; i++) {
    Elf64_Phdr *elf_phdr = (Elf64_Phdr *)((size_t)out + elf_ehdr->e_phoff +
                                          i * elf_ehdr->e_phentsize);
    if (elf_phdr->p_type == 7)
      tls = elf_phdr->p_vaddr;
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

    // wtf is this?
    // uint64_t file_start = (elf_phdr->p_vaddr & ~0xFFF) + elf_phdr->p_filesz;
    // uint64_t file_end = (elf_phdr->p_vaddr & ~0xFFF) + pagesRequired *
    // 0x1000; memset(file_start, 0, file_end - file_start);

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
      taskCreate(id, (uint64_t)elf_ehdr->e_entry, false, pagedir, argc, argv);

  if (tls)
    target->fsbase = tls;

  // User stack generation: the stack itself, AUXs, etc...
  stackGenerateUser(target, argc, argv, out, filesize, elf_ehdr);
  // free(out); // fix malloc

  // Current working directory init
  target->cwd = (char *)malloc(2);
  target->cwd[0] = '/';
  target->cwd[1] = '\0';

  taskCreateFinish(target);

  return id;
}
