#include "types.h"

typedef uint16_t Elf32_Half;  // Unsigned half int
typedef uint32_t Elf32_Off;   // Unsigned offset
typedef uint32_t Elf32_Addr;  // Unsigned address
typedef uint32_t Elf32_Word;  // Unsigned int
typedef int32_t  Elf32_Sword; // Signed int

#define ELF_NIDENT 16

typedef struct {
  uint8_t    e_ident[ELF_NIDENT];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off  e_phoff;
  Elf32_Off  e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  Elf32_Word p_type;
  Elf32_Off  p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct {
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off  sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

enum Elf_Ident {
  EI_MAG0 = 0,       // 0x7F
  EI_MAG1 = 1,       // 'E'
  EI_MAG2 = 2,       // 'L'
  EI_MAG3 = 3,       // 'F'
  EI_CLASS = 4,      // Architecture (32/64)
  EI_DATA = 5,       // Byte Order
  EI_VERSION = 6,    // ELF Version
  EI_OSABI = 7,      // OS Specific
  EI_ABIVERSION = 8, // OS Specific
  EI_PAD = 9         // Padding
};

#define ELFMAG0 0x7F // e_ident[EI_MAG0]
#define ELFMAG1 'E'  // e_ident[EI_MAG1]
#define ELFMAG2 'L'  // e_ident[EI_MAG2]
#define ELFMAG3 'F'  // e_ident[EI_MAG3]

#define ELFDATA2LSB (1)   // Little Endian
#define ELFCLASS32 (1)    // 32-bit Architecture
#define ELF_x86_MACHINE 3 // 32-bit Architecture (later)

enum Elf_Type {
  ET_NONE = 0, // Unkown Type
  ET_REL = 1,  // Relocatable File
  ET_EXEC = 2  // Executable File
};

#ifndef ELF_H
#define ELF_H

bool     elf_check_file(Elf32_Ehdr *hdr);
int16_t  create_taskid();
uint32_t elf_execute(char *filepath, uint32_t argc, char **argv);

#endif