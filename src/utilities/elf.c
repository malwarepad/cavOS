#include "../../include/elf.h"

// ELF parser
// Copyright (C) 2023 Panagiotis

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
  return true;
}