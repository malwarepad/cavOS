#include <elf.h>
#include <fat32.h>
#include <paging.h>
#include <task.h>
#include <util.h>

// ELF parser
// Copyright (C) 2023 Panagiotis

#define ELF_DEBUG 0

bool elf_check_file(Elf32_Ehdr *hdr) {
  if (!hdr)
    return false;
  if (hdr->e_ident[EI_MAG0] != ELFMAG0) {
    debugf("ELF Header EI_MAG0 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG1] != ELFMAG1) {
    debugf("ELF Header EI_MAG1 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG2] != ELFMAG2) {
    debugf("ELF Header EI_MAG2 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_MAG3] != ELFMAG3) {
    debugf("ELF Header EI_MAG3 incorrect.\n");
    return false;
  }
  if (hdr->e_ident[EI_CLASS] != ELFCLASS32 ||
      hdr->e_machine != ELF_x86_MACHINE) {
    debugf("ELF architecture is not supported.\n");
    return false;
  }
  return true;
}

uint32_t elf_execute(char *filepath) {
  taskSwitchSpinlock = true;

  // Open & read executable file
  FAT32_Directory *dir = (FAT32_Directory *)malloc(sizeof(FAT32_Directory));
  if (!openFile(dir, filepath)) {
    debugf("Could not open %s\n", filepath);
    return 0;
  }
#if ELF_DEBUG
  debugf("[executing %s] filesize=%d\n", filepath, dir->filesize);
#endif
  uint8_t *out = (uint8_t *)malloc(dir->filesize);
  readFileContents(&out, dir);

  // Cast ELF32 header
  Elf32_Ehdr *elf_ehdr = (Elf32_Ehdr *)(out);

  if (!elf_check_file(elf_ehdr)) {
    debugf("File %s is not a valid cavOS ELF32 executable!\n", filepath);
    return 0;
  }

  // Create a new page directory which is later used by the process
  uint32_t *oldpagedir = GetPageDirectory();
  uint32_t *pagedir = PageDirectoryAllocate();
  ChangePageDirectory(pagedir);

#if ELF_DEBUG
  debugf("\n[elf_ehdr] entry=%x type=%d arch=%d\n", elf_ehdr->e_entry,
         elf_ehdr->e_type, elf_ehdr->e_machine);
  debugf("[early phdr] offset=%x count=%d size=%d\n", elf_ehdr->e_phoff,
         elf_ehdr->e_phnum, elf_ehdr->e_phentsize);
#endif

  int32_t id = create_taskid();
  if (id == -1) {
    debugf("Cannot fetch task id... You probably reached the task limit!");
    printf("Cannot fetch task id... You probably reached the task limit!");
    panic();
  }

  // Loop through the multiple ELF32 program header tables
  for (int i = 0; i < elf_ehdr->e_phnum; i++) {
    Elf32_Phdr *elf_phdr = (Elf32_Phdr *)((uint32_t)out + elf_ehdr->e_phoff +
                                          i * elf_ehdr->e_phentsize);
    if (elf_phdr->p_type != PT_LOAD)
      continue;

    // Map the (current) program page
    uint32_t pagesRequired = DivRoundUp(elf_phdr->p_memsz, 0x1000);
    for (int j = 0; j < pagesRequired; j++) {
      uint32_t vaddr = (elf_phdr->p_vaddr & ~0xFFF) + j * 0x1000;
      VirtualMap(vaddr, BitmapAllocatePageframe(),
                 PAGE_FLAG_OWNER | PAGE_FLAG_USER | PAGE_FLAG_WRITE);
    }

    // Copy the required info
    memcpy(elf_phdr->p_vaddr, out + elf_phdr->p_offset, elf_phdr->p_filesz);

    uint32_t file_start = (elf_phdr->p_vaddr & ~0xFFF) + elf_phdr->p_filesz;
    uint32_t file_end = (elf_phdr->p_vaddr & ~0xFFF) + pagesRequired * 0x1000;
    memset(file_start, 0, file_end - file_start);

#if ELF_DEBUG
    debugf("[phdr] type=%d offset=%x vaddr=%x size=%x alignment=%x\n",
           elf_phdr->p_type, elf_phdr->p_offset, elf_phdr->p_vaddr,
           elf_phdr->p_memsz, elf_phdr->p_align);
#endif
  }

  // For the foreseeable future ;)
#if ELF_DEBUG
  for (int i = 0; i < elf_ehdr->e_shnum; i++) {
    Elf32_Shdr *elf_shdr = (Elf32_Shdr *)((uint32_t)out + elf_ehdr->e_shoff +
                                          i * elf_ehdr->e_shentsize);
    debugf("[shdr] type=%d offset=%x\n", elf_shdr->sh_type,
           elf_shdr->sh_offset);
  }
#endif

  // Map the user stack (for variables & such)
  for (int i = 0; i < USER_STACK_PAGES; i++) {
    VirtualMap(USER_STACK_BOTTOM - USER_STACK_PAGES * 0x1000 + i * 0x1000,
               BitmapAllocatePageframe(),
               PAGE_FLAG_OWNER | PAGE_FLAG_USER | PAGE_FLAG_WRITE);
  }

#if ELF_DEBUG
  debugf("[new pagedir] offset=%x\n", pagedir);
#endif

  create_task(id, (uint32_t)elf_ehdr->e_entry, true, pagedir);

  // Cleanup...
  free(out);
  free(dir);
  ChangePageDirectory(oldpagedir);

  taskSwitchSpinlock = false;

  return id;
}
